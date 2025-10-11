// scheduler.cpp
// Implements FCFS multithreaded scheduling with timestamps
// Compile together with server.cpp, server-worker.cpp, requestresponse.cpp, file-io.cpp, conf.cpp

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <signal.h>

#include "requestresponse.h"
#include "server-worker.cpp"  // your ServerWorker
#include "file-io.h"          // read_file/write_file functions

using namespace std;
using namespace std::chrono;

// --------------------------- Utility Functions ---------------------------

static atomic<bool> g_running(true);

void handle_sigint_fcfs(int) {
    g_running.store(false);
}

// read until newline '\n' (client sends JSON followed by '\n')
static string recv_json_line_full(int sock, int max_total = 10 * 1024 * 1024) {
    string out;
    char buf[4096];
    while (true) {
        ssize_t n = recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) {
            if (out.empty()) return string(); // connection closed before data
            else break;
        }
        out.append(buf, buf + n);
        auto pos = out.find('\n');
        if (pos != string::npos) {
            out.resize(pos);
            break;
        }
        if ((int)out.size() >= max_total) break; // safety
    }
    return out;
}

static long now_mono_ms() {
    return (long)duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()).count();
}

// --------------------------- FCFS Scheduler ------------------------------

class FCFSScheduler {
public:
    int worker_threads;
    int packet_size;

    FCFSScheduler(int server_threads, int p)
        : worker_threads(server_threads), packet_size(p) {
        signal(SIGINT, handle_sigint_fcfs);
    }

    void server_loop(const string &server_ip, int server_port) {
        cout << "[FCFS] Starting server on " << server_ip << ":" << server_port << endl;

        // ---------- Setup server socket ----------
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            cerr << "[FCFS] socket() failed\n";
            exit(1);
        }

        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(server_port);
        addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

        if (bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            cerr << "[FCFS] bind() failed\n";
            close(server_socket);
            exit(1);
        }

        if (listen(server_socket, 50) < 0) {
            cerr << "[FCFS] listen() failed\n";
            close(server_socket);
            exit(1);
        }

        cout << "[FCFS] Listening...\n";

        // ---------- Shared request queue ----------
        deque<pair<int, RequestResponse>> request_queue;
        mutex queue_mtx;
        condition_variable queue_cv;

        atomic<bool> accepting{true};

        // ---------- Worker threads ----------
        vector<thread> workers;
        for (int i = 0; i < worker_threads; ++i) {
            workers.emplace_back([&, i]() {
                //ServerWorker worker(packet_size);
                ServerWorker worker(packet_size, file_store_dir);

                while (g_running.load()) {
                    pair<int, RequestResponse> job;
                    {
                        unique_lock<mutex> lock(queue_mtx);
                        queue_cv.wait(lock, [&]() {
                            return !request_queue.empty() || !g_running.load();
                        });
                        if (!g_running.load() && request_queue.empty()) return;
                        job = request_queue.front();
                        request_queue.pop_front();
                    }

                    RequestResponse req = job.second;
                    int conn = job.first;

                    // Service start
                    req.service_start_time = now_mono_ms();

                    // Process request (handle read/write)
                    worker.process_req(conn, req);

                    // Completion
                    req.completion_time = now_mono_ms();

                    // Send back timing info (optional)
                    try {
                        RequestResponse timing_resp = req;
                        timing_resp.req_status = true;
                        string line = timing_resp.convert_to_json() + "\n";
                        send(conn, line.c_str(), line.size(), 0);
                    } catch (...) {}

                    close(conn);
                }
            });
        }

        // ---------- Accept loop ----------
        while (g_running.load()) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int conn = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (conn < 0) {
                if (!g_running.load()) break;
                cerr << "[FCFS] accept() failed\n";
                continue;
            }

            string json = recv_json_line_full(conn);
            if (json.empty()) {
                cerr << "[FCFS] Empty or invalid request, closing\n";
                close(conn);
                continue;
            }

            RequestResponse req;
            try {
                req.convert_from_json(json);
            } catch (...) {
                cerr << "[FCFS] Failed to parse JSON, closing\n";
                close(conn);
                continue;
            }

            req.arrival_time = now_mono_ms();

            {
                lock_guard<mutex> lock(queue_mtx);
                request_queue.push_back({conn, req});
            }
            queue_cv.notify_one();
        }

        // ---------- Shutdown ----------
        g_running.store(false);
        queue_cv.notify_all();
        close(server_socket);

        for (auto &t : workers) if (t.joinable()) t.join();
        cout << "[FCFS] Server shutdown complete.\n";
    }
};

