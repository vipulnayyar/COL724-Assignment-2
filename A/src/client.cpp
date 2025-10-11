#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>

#include "rapidjson/document.h"
#include "requestresponse.cpp"

using namespace std;
using namespace rapidjson;

// helper: send all bytes
bool send_all(int sock, const string &data) {
    size_t total_sent = 0;
    while (total_sent < data.size()) {
        ssize_t sent = send(sock, data.c_str() + total_sent, data.size() - total_sent, 0);
        if (sent <= 0) return false;
        total_sent += sent;
    }
    return true;
}

// helper: receive response (until socket closes)
string recv_all(int sock) {
    string response;
    char buffer[4096];
    ssize_t bytes;
    while ((bytes = recv(sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes] = '\0';
        response += buffer;
    }
    return response;
}

// helper: read file into string
string read_file(const string &path) {
    ifstream file(path);
    if (!file.is_open()) {
        cerr << "Error: cannot open " << path << endl;
        exit(1);
    }
    return string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
}

int main(int argc, char *argv[]) {
    string config_path = "";
    string op = "";
    string file_path = "";
    string server_filename = "";
    int packet_lines = 1;

    // --- Parse command-line args ---
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) config_path = argv[++i];
        else if (arg == "--op" && i + 1 < argc) op = argv[++i];
        else if (arg == "--file" && i + 1 < argc) file_path = argv[++i];
        else if (arg == "--name" && i + 1 < argc) server_filename = argv[++i];
        else if (arg == "--p" && i + 1 < argc) packet_lines = stoi(argv[++i]);
    }

    if (config_path.empty()) {
        cerr << "Error: --config required\n";
        return 1;
    }
    if (op != "PUT" && op != "GET") {
        cerr << "Error: --op must be PUT or GET\n";
        return 1;
    }

    // --- Load config.json ---
    ifstream cfg_file(config_path);
    if (!cfg_file.is_open()) {
        cerr << "Error: cannot open config file\n";
        return 1;
    }
    string cfg_json((istreambuf_iterator<char>(cfg_file)), istreambuf_iterator<char>());
    Document cfg;
    cfg.Parse(cfg_json.c_str());
    if (cfg.HasParseError() || !cfg.IsObject()) {
        cerr << "Error: invalid JSON config\n";
        return 1;
    }
    if (!cfg.HasMember("server_ip") || !cfg.HasMember("server_port")) {
        cerr << "Error: missing server_ip or server_port in config\n";
        return 1;
    }

    string server_ip = cfg["server_ip"].GetString();
    int server_port = cfg["server_port"].GetInt();

    // --- Build request JSON ---
    RequestResponse req;
    req.req_id = 1;
    req.req_status = false;
    req.file_path = (op == "PUT") ? server_filename : file_path;
    req.read_line_index = 1;
    req.write_num_lines = packet_lines;
    req.req_type = (op == "PUT") ? "write" : "read";

    if (op == "PUT") {
        req.write_data = read_file(file_path);
    }

    string request_json = req.convert_to_json();

    // --- Connect to server ---
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket creation failed\n";
        return 1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid server IP\n";
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection to server failed\n";
        return 1;
    }

    // --- Send and receive ---
    request_json += "\n";  // end with newline for server to parse
    if (!send_all(sock, request_json)) {
        cerr << "Send failed\n";
        return 1;
    }

    string response = recv_all(sock);
    cout << "Server response:\n" << response << endl;

    close(sock);
    return 0;
}

