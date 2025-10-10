// client.cpp
// Compile: g++ -std=c++17 -pthread client.cpp requestresponse.cpp -o client

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream> 
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

#include "rapidjson/document.h"
#include "requestresponse.h" // use the provided RequestResponse
using namespace std::chrono;
using namespace std;
using namespace rapidjson;
using namespace std::chrono;

steady_clock::time_point program_start;

long now_ms() {
    return duration_cast<milliseconds>(
        steady_clock::now() - program_start).count();
}



// ------------------ config & globals ------------------
string SERVER_IP = "127.0.0.1";
int SERVER_PORT = 9000;
int DEFAULT_CLIENT_THREADS = 1;

atomic<int> global_req_id{0};

struct CSVRow {
    int req_id;
    string req_type;      // "read" or "write"
    string file_path;
    long client_send_ms;
    long client_complete_ms;
    int server_arrival;   // as returned by server (if any)
    int server_start;
    int server_finish;
    long response_time_ms;
    long waiting_time_ms;
};

vector<CSVRow> g_results;
mutex g_results_mtx;



bool send_all(int sock, const char* buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(sock, buf + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += (size_t)n;
    }
    return true;
}
bool send_all(int sock, const string &s) {
    return send_all(sock, s.c_str(), s.size());
}

// Read a single JSON-line reply (terminated by '\n'). Returns empty string on error/timeout.
string recv_json_line(int sock, int timeout_sec) {
    // set recv timeout
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    string line;
    char ch;
    while (true) {
        ssize_t n = recv(sock, &ch, 1, 0);
        if (n == 0) {
            // peer closed
            break;
        }
        if (n < 0) {
            // error or timeout
            break;
        }
        if (ch == '\n') break;
        line.push_back(ch);
    }
    return line;
}

int connect_to_server(const string &ip, int port, int timeout_sec=5) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    // optional: set connect timeout via non-blocking + select, but keep simple here
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        close(sock);
        return -1;
    }
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

string read_file_to_string(const string &path) {
    ifstream ifs(path, ios::binary);
    if (!ifs) {
        throw runtime_error("Cannot open file: " + path);
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

// ------------------ request processing ------------------
// send a single write (PUT) request, capture timestamps & server response
bool do_put_single(int req_id, const string &localpath, int packet_p, CSVRow &out, int recv_timeout=10) {
    RequestResponse req;
    req.req_id = req_id;
    req.req_type = "write";
    req.file_path = localpath; // server-worker expects file_path -> name; partner's server may save this name
    req.read_line_index = 0;
    req.read_num_lines = 0;
    req.req_status = false;

    try {
        req.write_data = read_file_to_string(localpath);
    } catch (exception &e) {
        cerr << "PUT: cannot read local file '" << localpath << "': " << e.what() << endl;
        return false;
    }

    // record client send time
    out.client_send_ms = now_ms();

    // connect
    int sock = connect_to_server(SERVER_IP, SERVER_PORT);
    if (sock < 0) {
        cerr << "PUT: connection failed to " << SERVER_IP << ":" << SERVER_PORT << endl;
        return false;
    }

    string json_line = req.convert_to_json() + "\n";
    if (!send_all(sock, json_line)) {
        cerr << "PUT: send failed\n";
        close(sock);
        return false;
    }

    // read server reply (single JSON)
    string reply = recv_json_line(sock, recv_timeout);
    if (reply.empty()) {
        cerr << "PUT: no reply or timed out\n";
        close(sock);
        return false;
    }

    RequestResponse resp;
    try {
        resp.convert_from_json(reply);
    } catch (...) {
        // parse error — still store client completion time
    }

    // client completion timestamp
    out.client_complete_ms = now_ms();

    // server timestamps (if present)
    out.server_arrival = resp.arrival_time;
    out.server_start = resp.service_start_time;
    out.server_finish = resp.completion_time;

    // compute response/waiting times using server timestamps if available
    if (out.server_arrival > 0 && out.server_finish > 0) {
        out.response_time_ms = out.server_finish - out.server_arrival;
    } else {
        out.response_time_ms = out.client_complete_ms - out.client_send_ms;
    }
    if (out.server_arrival > 0 && out.server_start > 0) {
        out.waiting_time_ms = out.server_start - out.server_arrival;
    } else out.waiting_time_ms = 0;

    close(sock);
    return true;
}

// send a single read (GET) request, accumulate read_data, capture timestamps & server replies (may be multiple JSON packets)
bool do_get_single(int req_id, const string &server_filename, int packet_p, CSVRow &out, int recv_timeout=10) {
    RequestResponse req;
    req.req_id = req_id;
    req.req_type = "read";
    req.file_path = server_filename;
    req.read_line_index = 1;
    req.read_num_lines = packet_p;
    req.write_data = "";
    req.req_status = false;

    out.client_send_ms = now_ms();

    int sock = connect_to_server(SERVER_IP, SERVER_PORT);
    if (sock < 0) {
        cerr << "GET: connection failed to " << SERVER_IP << ":" << SERVER_PORT << endl;
        return false;
    }

    string json_line = req.convert_to_json() + "\n";
    if (!send_all(sock, json_line)) {
        cerr << "GET: send failed\n";
        close(sock);
        return false;
    }

    // Server may send multiple JSON packets. We'll loop until:
    // - server signals req_status true, OR
    // - we get no data (connection closed), OR
    // - timeout occurs
    string accumulated;
    RequestResponse resp;
    bool gotAny = false;
    while (true) {
        string reply = recv_json_line(sock, recv_timeout);
        if (reply.empty()) break;
        gotAny = true;
        try {
            resp.convert_from_json(reply);
        } catch (...) {}
        if (!resp.read_data.empty()) {
            accumulated += resp.read_data;
            // server may send newline-separated lines inside read_data
        }
        // if server indicates completion in req_status, break
        if (resp.req_status) break;
        // else continue to read next packet
    }

    // write accumulated data to local file for verification
    string outname = string("client_download_") + server_filename;
    ofstream ofs(outname, ios::binary);
    if (ofs) ofs << accumulated;
    ofs.close();

    out.client_complete_ms = now_ms();
    out.server_arrival = resp.arrival_time;
    out.server_start = resp.service_start_time;
    out.server_finish = resp.completion_time;

    if (out.server_arrival > 0 && out.server_finish > 0) {
        out.response_time_ms = out.server_finish - out.server_arrival;
    } else {
        out.response_time_ms = out.client_complete_ms - out.client_send_ms;
    }
    if (out.server_arrival > 0 && out.server_start > 0) {
        out.waiting_time_ms = out.server_start - out.server_arrival;
    } else out.waiting_time_ms = 0;

    close(sock);
    return gotAny;
}

// ------------------ worker thread ------------------
void worker_thread(const vector<pair<string,string>> &commands, int packet_p, int iterations) {
    for (int it = 0; it < iterations; ++it) {
        for (auto &c : commands) {
            string op = c.first;
            string arg = c.second;
            int req_id = ++global_req_id;

            CSVRow r;
            r.req_id = req_id;
            r.req_type = (op == "PUT") ? "write" : "read";
            r.file_path = arg;
            r.client_send_ms = r.client_complete_ms = 0;
            r.server_arrival = r.server_start = r.server_finish = 0;
            r.response_time_ms = r.waiting_time_ms = 0;

            bool ok = false;
            if (op == "PUT") {
                ok = do_put_single(req_id, arg, packet_p, r);
            } else { // GET
                ok = do_get_single(req_id, arg, packet_p, r);
            }

            {
                lock_guard<mutex> lk(g_results_mtx);
                g_results.push_back(r);
            }

            if (!ok) {
                cerr << "Request failed: " << op << " " << arg << " (req_id=" << req_id << ")\n";
            }
        }
    }
}

// ------------------ main & CLI ------------------
void print_usage(const char *p) {
    cerr << "Usage examples:\n";
    cerr << p << " --config config.json --mode single --op PUT --file ./data/test.txt --name test.txt --p 5\n";
    cerr << p << " --config config.json --mode single --op GET --name test.txt --p 5\n";
    cerr << p << " --config config.json --mode batch --commands cmds.txt --client-threads 8 --iterations 10 --p 5 --output results.csv\n";
}

int main(int argc, char** argv) {
    string config_file;
    string mode = "single"; // single or batch
    string op;
    string filearg;
    string namearg;
    string commands_file;
    int packet_p = 1;
    int client_threads = -1; // override from config if set
    int iterations = 1;
    string output_csv = "client_results.csv";
    bool dry_run = false;

    // parse CLI
    for (int i=1;i<argc;i++){
        string a = argv[i];
        if (a=="--config" && i+1<argc) config_file = argv[++i];
        else if (a=="--mode" && i+1<argc) mode = argv[++i];
        else if (a=="--op" && i+1<argc) op = argv[++i];
        else if (a=="--file" && i+1<argc) filearg = argv[++i];
        else if (a=="--name" && i+1<argc) namearg = argv[++i];
        else if (a=="--commands" && i+1<argc) commands_file = argv[++i];
        else if (a=="--p" && i+1<argc) packet_p = stoi(argv[++i]);
        else if (a=="--client-threads" && i+1<argc) client_threads = stoi(argv[++i]);
        else if (a=="--iterations" && i+1<argc) iterations = stoi(argv[++i]);
        else if (a=="--output" && i+1<argc) output_csv = argv[++i];
        else if (a=="--dry-run") dry_run = true;
        else if (a=="--help") { print_usage(argv[0]); return 0; }
    }

    if (config_file.empty()) {
        cerr << "--config <file> is required\n";
        return 1;
    }
    
    // set program start time here
    program_start = steady_clock::now();

    // read config.json
    ifstream ifs(config_file);
    if (!ifs) { cerr << "Cannot open config file\n"; return 1; }
    string cfgs((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
    Document cfg; cfg.Parse(cfgs.c_str());
    if (cfg.HasParseError() || !cfg.IsObject()) { cerr << "Invalid config.json\n"; return 1; }
    if (!cfg.HasMember("server_ip") || !cfg.HasMember("server_port") || !cfg.HasMember("client_threads")) {
        cerr << "config.json missing required fields (server_ip, server_port, client_threads)\n"; 
        return 1;
    }
    SERVER_IP = cfg["server_ip"].GetString();
    SERVER_PORT = cfg["server_port"].GetInt();
    DEFAULT_CLIENT_THREADS = cfg["client_threads"].GetInt();
    if (client_threads > 0) DEFAULT_CLIENT_THREADS = client_threads;

    // build commands
    vector<pair<string,string>> commands;
    if (mode == "single") {
        if (op.empty()) { cerr << "--op required in single mode\n"; return 1; }
        if (op=="PUT") {
            if (filearg.empty() || namearg.empty()) { cerr << "PUT requires --file and --name\n"; return 1; }
            // local file path -> provide local path to put; set server filename as basename in our PUT function
            // We'll include server filename as argument by concatenating "localpath|servername", but simpler: use localpath and server stores with basename
            commands.emplace_back("PUT", filearg);
        } else if (op=="GET") {
            if (namearg.empty()) { cerr << "GET requires --name\n"; return 1; }
            commands.emplace_back("GET", namearg);
        } else { cerr << "Unknown op\n"; return 1; }
    } else {
        // batch mode: parse commands file
        if (commands_file.empty()) { cerr << "--commands required in batch mode\n"; return 1; }
        ifstream cfs(commands_file);
        if (!cfs) { cerr << "Cannot open commands file\n"; return 1; }
        string line;
        while (getline(cfs, line)) {
            if (line.empty()) continue;
            istringstream iss(line);
            string a,b; iss >> a >> b;
            if (a != "PUT" && a != "GET") { cerr << "Skipping invalid command: " << line << "\n"; continue; }
            if (b.empty()) { cerr << "Skipping invalid command line: " << line << "\n"; continue; }
            commands.emplace_back(a,b);
        }
        if (commands.empty()) { cerr << "No valid commands found\n"; return 1; }
    }

    // show summary and dry-run
    cout << "Client config: server " << SERVER_IP << ":" << SERVER_PORT << " threads=" << DEFAULT_CLIENT_THREADS << "\n";
    cout << "Mode: " << mode << " , commands count = " << commands.size() << " , iterations = " << iterations << " , p = " << packet_p << "\n";
    if (dry_run) {
        cout << "[Dry-run] Will NOT contact server. Example request JSON for first command:\n";
        RequestResponse tmp;
        tmp.req_id = 1; tmp.req_status = false;
        if (commands[0].first == "PUT") {
            tmp.req_type = "write";
            tmp.file_path = commands[0].second;
            try { tmp.write_data = read_file_to_string(commands[0].second); }
            catch(...) { tmp.write_data = ""; }
        } else {
            tmp.req_type = "read";
            tmp.file_path = commands[0].second;
            tmp.read_line_index = 1; tmp.read_num_lines = packet_p;
        }
        cout << tmp.convert_to_json() << "\n";
        return 0;
    }

    
    vector<thread> threads;
    for (int i=0;i<DEFAULT_CLIENT_THREADS;i++){
        threads.emplace_back(worker_thread, std::cref(commands), packet_p, iterations);
    }
    for (auto &t : threads) t.join();

    // write CSV
    ofstream ofs(output_csv);
    if (!ofs) { cerr << "Cannot open output CSV file for writing\n"; return 1; }
    ofs << "req_id,req_type,file_path,client_send_ms,client_complete_ms,server_arrival,server_start,server_finish,response_time_ms,waiting_time_ms\n";
    {
        lock_guard<mutex> lg(g_results_mtx);
        for (auto &r : g_results) {
            ofs << r.req_id << "," << r.req_type << "," << r.file_path << ",";
            ofs << r.client_send_ms << "," << r.client_complete_ms << ",";
            if (r.server_arrival != 0) ofs << r.server_arrival << ","; else ofs << ",";
            if (r.server_start != 0) ofs << r.server_start << ","; else ofs << ",";
            if (r.server_finish != 0) ofs << r.server_finish << ","; else ofs << ",";
            ofs << r.response_time_ms << "," << r.waiting_time_ms << "\n";
        }
    }
    ofs.close();
    cout << "Wrote results to " << output_csv << " (rows=" << g_results.size() << ")\n";
    return 0;
}

