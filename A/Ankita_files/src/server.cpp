#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h> 

#include "CLI/CLI.hpp"
using namespace std;

#include "conf.cpp"
#include "requestresponse.h"
#include "server-worker.cpp"

// -----------------------------------------------------------------------------
// Global Queue & Synchronization
// -----------------------------------------------------------------------------
queue<pair<int, RequestResponse>> request_queue;
mutex queue_mutex;
condition_variable queue_cv;
atomic<bool> server_running(true);

// -----------------------------------------------------------------------------
// FCFS Scheduler
// -----------------------------------------------------------------------------
class FCFSScheduler {
    int server_socket;
    int server_threads;
    int packet_size;
    vector<thread> worker_threads;

public:
    FCFSScheduler(int threads, int p)
        : server_threads(threads), packet_size(p) {}

    // Worker function: processes requests in FCFS order
    void worker_loop() {
        ServerWorker worker(packet_size);

        while (server_running) {
            unique_lock<mutex> lock(queue_mutex);
            queue_cv.wait(lock, [] { return !request_queue.empty() || !server_running; });

            if (!server_running && request_queue.empty()) break;

            auto [conn, req] = request_queue.front();
            request_queue.pop();
            lock.unlock();

            // ---------------- Timestamp: start ----------------
            req.service_start_time = chrono::duration_cast<chrono::milliseconds>(
                chrono::steady_clock::now().time_since_epoch()).count();

            worker.process_req(conn, req);

            // ---------------- Timestamp: finish ----------------
            req.completion_time = chrono::duration_cast<chrono::milliseconds>(
                chrono::steady_clock::now().time_since_epoch()).count();

            double response_time = (req.completion_time - req.arrival_time) / 1000.0;
            cout << "[INFO] Processed ReqID=" << req.req_id
                 << " | Response Time=" << response_time << " s" << endl;

            close(conn);
        }
    }

    // TCP server accept loop
    void server_loop(string server_ip, int server_port) {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            cerr << "[ERROR] Socket creation failed!" << endl;
            exit(1);
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
        server_addr.sin_port = htons(server_port);

        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "[ERROR] Bind failed!" << endl;
            close(server_socket);
            exit(1);
        }

        if (listen(server_socket, 10) < 0) {
            cerr << "[ERROR] Listen failed!" << endl;
            close(server_socket);
            exit(1);
        }

        cout << "Server listening on " << server_ip << ":" << server_port << endl;

        // Start worker threads
        for (int i = 0; i < server_threads; i++) {
            worker_threads.emplace_back(&FCFSScheduler::worker_loop, this);
        }

        // Accept connections
        while (server_running) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int conn = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (conn < 0) {
                cerr << "[WARN] Accept failed!" << endl;
                continue;
            }

            // Read request JSON
            char buffer[8192];
            int bytes = recv(conn, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) {
                cerr << "[WARN] Empty or failed recv()" << endl;
                close(conn);
                continue;
            }

            buffer[bytes] = '\0';
            string json_req(buffer);

            RequestResponse req;
            try {
                req.convert_from_json(json_req);
            } catch (...) {
                cerr << "[ERROR] Failed to parse JSON request." << endl;
                close(conn);
                continue;
            }

            // ---------------- Timestamp: arrival ----------------
            req.arrival_time = chrono::duration_cast<chrono::milliseconds>(
                chrono::steady_clock::now().time_since_epoch()).count();

            cout << "[INFO] Received request ID=" << req.req_id
                 << " Type=" << req.req_type
                 << " at t=" << req.arrival_time << " ms" << endl;

            // Push to queue
            {
                lock_guard<mutex> lock(queue_mutex);
                request_queue.push({conn, req});
            }
            queue_cv.notify_one();
        }

        // Graceful shutdown
        for (auto &t : worker_threads) {
            if (t.joinable()) t.join();
        }

        close(server_socket);
    }
};

// -----------------------------------------------------------------------------
// MAIN FUNCTION
// -----------------------------------------------------------------------------
int main(int argc, char** argv) {
    CLI::App app{"Server Application"};

    string config_file;
    app.add_option("--config-file", config_file, "(MANDATORY) Path to JSON Config file")
        ->required()
        ->check(CLI::ExistingFile);

    string sched_policy;
    app.add_option("--sched", sched_policy, "(MANDATORY) Scheduling policy for server")
        ->check(CLI::IsMember({"fcfs", "sjf", "rr"}))
        ->required();

    string file_store_dir;
    app.add_option("--file", file_store_dir, "(MANDATORY) Directory path for file data store")
        ->check(CLI::ExistingDirectory)
        ->required();

    int packet_size;
    app.add_option("--p", packet_size, "(MANDATORY) Lines of text for server to send in one packet")
        ->required()
        ->check(CLI::Range(1, std::numeric_limits<int>::max()));

    try {
        CLI11_PARSE(app, argc, argv);
    } catch (const CLI::Error &err) {
        return app.exit(err);
    }

    // Load config.json
    if (load_config(config_file) != 0) {
        cerr << "[ERROR] Config load failure" << endl;
        return -1;
    }

    cout << "\nSERVER CONFIG:\n";
    cout << "SERVER_IP       : " << SERVER_IP << endl;
    cout << "SERVER_PORT     : " << SERVER_PORT << endl;
    cout << "SERVER_THREADS  : " << SERVER_THREADS << endl;
    cout << "CLIENT_THREADS  : " << CLIENT_THREADS << endl;
    cout << "Sched Policy    : " << sched_policy << endl;
    cout << "File Store Dir  : " << file_store_dir << endl;
    cout << "Packet Size     : " << packet_size << endl;

    // Policy selection
    if (sched_policy == "fcfs") {
        cout << "\nStarting FCFS Scheduler..." << endl;
        FCFSScheduler scheduler(SERVER_THREADS, packet_size);
        scheduler.server_loop(SERVER_IP, SERVER_PORT);
    } else {
        cerr << "Currently only FCFS is implemented." << endl;
        return -1;
    }

    cout << "\n[INFO] Exiting Server application" << endl;
    return 0;
}

