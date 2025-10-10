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
#include "CLI/CLI.hpp"

using namespace std;

#include "conf.cpp"
#include "requestresponse.h"
#include "server-worker.cpp"

// ------------------------ Global Queue and Sync Primitives ------------------------
queue<pair<int, RequestResponse>> request_queue;
mutex queue_mutex;
condition_variable queue_cv;
atomic<bool> server_running(true);

// ------------------------ FCFS Scheduler Class ------------------------
class FCFSScheduler {
    int server_socket;
    int server_threads;
    int packet_size;
    vector<thread> worker_threads;

public:
    FCFSScheduler(int threads, int p) {
        server_threads = threads;
        packet_size = p;
    }

    // Worker function (thread pool)
    void worker_loop() {
        ServerWorker worker(packet_size);
        while (server_running) {
            unique_lock<mutex> lock(queue_mutex);
            queue_cv.wait(lock, [] { return !request_queue.empty() || !server_running; });
            if (!server_running) break;

            auto [conn, req] = request_queue.front();
            request_queue.pop();
            lock.unlock();

            // Timestamp: start
            auto start_time = chrono::steady_clock::now();

            worker.process_req(conn, req);

            // Timestamp: finish
            auto finish_time = chrono::steady_clock::now();

            double response_time = chrono::duration<double>(finish_time - start_time).count();
            cout << "[INFO] Request processed | Response Time: " << response_time << " s" << endl;

            close(conn);
        }
    }

    // Start the TCP server and accept incoming clients
    void server_loop(string server_ip, int server_port) {
        // Create socket
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            cerr << "Socket creation failed!" << endl;
            exit(1);
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
        server_addr.sin_port = htons(server_port);

        // Bind
        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "Bind failed!" << endl;
            exit(1);
        }

        // Listen
        if (listen(server_socket, 10) < 0) {
            cerr << "Listen failed!" << endl;
            exit(1);
        }

        cout << "Server listening on " << server_ip << ":" << server_port << endl;

        // Start worker threads
        for (int i = 0; i < server_threads; i++) {
            worker_threads.emplace_back(&FCFSScheduler::worker_loop, this);
        }

        // Accept loop
        while (server_running) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int conn = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (conn < 0) {
                cerr << "Accept failed!" << endl;
                continue;
            }

            // Read request
            char buffer[4096];
            int bytes = recv(conn, buffer, sizeof(buffer), 0);
            if (bytes <= 0) {
                close(conn);
                continue;
            }

            string json_req(buffer, bytes);
            RequestResponse req;
            req.parse_from_json(json_req);

            // Timestamp: arrival
            auto arrival_time = chrono::steady_clock::now();
            cout << "[INFO] Request arrived at "
                 << chrono::duration_cast<chrono::milliseconds>(arrival_time.time_since_epoch()).count()
                 << " ms" << endl;

            {
                lock_guard<mutex> lock(queue_mutex);
                request_queue.push({conn, req});
            }
            queue_cv.notify_one();
        }

        // Cleanup
        for (auto& t : worker_threads) {
            if (t.joinable()) t.join();
        }

        close(server_socket);
    }
};

// ------------------------ MAIN FUNCTION ------------------------
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

    if (load_config(config_file) != 0) {
        cerr << "Config load failure\n";
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

    if (sched_policy == "fcfs") {
        cout << "\nStarting FCFS Scheduler..." << endl;
        FCFSScheduler scheduler(SERVER_THREADS, packet_size);
        scheduler.server_loop(SERVER_IP, SERVER_PORT);
    } else {
        cerr << "Currently only FCFS is implemented." << endl;
        return -1;
    }

    cout << "\nExiting Server application" << endl;
    return 0;
}

