#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <filesystem>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <assignment/requestresponse.h>

using namespace std;
using namespace rapidjson;
using namespace std::chrono;

// Helper function to get current timestamp in milliseconds
long long now_ms() {
    auto now = system_clock::now();
    return duration_cast<milliseconds>(now.time_since_epoch()).count();
}

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

void process_server_response(int sock) {
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE + 1];  // +1 for null terminator
    int total_bytes = 0;

    while (true) {
        ssize_t n = recv(sock, buffer, BUFFER_SIZE, 0);
        if (n <= 0) {
            if (n < 0) {
                cerr << "Error receiving data: " << strerror(errno) << endl;
            }
            break;  // Connection closed or error
        }

        // Null terminate and process the received data
        buffer[n] = '\0';
        cout << "Received " << n << " bytes: " << buffer << endl;
        total_bytes += n;
    }

    cout << "Total bytes received: " << total_bytes << endl;
}

// Helper function to read file contents
string read_file_contents(const string &path) {
    ifstream file(path, ios::binary);
    if (!file) {
        throw runtime_error("Cannot open file: " + path);
    }
    return string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
}

void print_usage(const char* program) {
    cerr << "Usage: " << program << " [OPTIONS]\n"
         << "Options:\n"
         << "  --op <GET|PUT>         Operation type (required)\n"
         << "  --file <path>          Local file path for PUT operation\n"
         << "  --name <name>          Server-side file name\n"
         << "  --config <path>        Path to config.json (default: ./conf/config.json)\n"
         << "Example:\n"
         << "  " << program << " --op GET --name test.txt\n"
         << "  " << program << " --op PUT --file ./local/test.txt --name test.txt\n";
}

int main(int argc, char** argv) {
    // Command line arguments
    string operation;
    string file_path;
    string file_name;
    string config_path = "./conf/config.json";

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--op" && i + 1 < argc) operation = argv[++i];
        else if (arg == "--file" && i + 1 < argc) file_path = argv[++i];
        else if (arg == "--name" && i + 1 < argc) file_name = argv[++i];
        else if (arg == "--config" && i + 1 < argc) config_path = argv[++i];
        else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    // Read config file
    ifstream config_file(config_path);
    if (!config_file.is_open()) {
        cerr << "Error: Cannot open config file: " << config_path << endl;
        return 1;
    }

    string config_str((istreambuf_iterator<char>(config_file)), istreambuf_iterator<char>());
    config_file.close();

    // Parse config JSON
    Document config;
    config.Parse(config_str.c_str());
    if (config.HasParseError() || !config.IsObject()) {
        cerr << "Error: Invalid JSON in config file" << endl;
        return 1;
    }

    // Get server details from config
    if (!config.HasMember("server_ip") || !config.HasMember("server_port")) {
        cerr << "Error: Config file missing required fields (server_ip, server_port)" << endl;
        return 1;
    }

    string SERVER_IP = config["server_ip"].GetString();
    int SERVER_PORT = config["server_port"].GetInt();

    // Validate arguments
    if (operation.empty()) {
        cerr << "Error: --op is required\n";
        print_usage(argv[0]);
        return 1;
    }

    if (operation != "GET" && operation != "PUT") {
        cerr << "Error: --op must be either GET or PUT\n";
        return 1;
    }

    if (operation == "PUT" && file_path.empty()) {
        cerr << "Error: --file is required for PUT operation\n";
        return 1;
    }

    if (file_name.empty()) {
        if (operation == "PUT") {
            // Use the base name of the file path as the server-side name
            size_t pos = file_path.find_last_of("/\\");
            file_name = pos == string::npos ? file_path : file_path.substr(pos + 1);
        } else {
            cerr << "Error: --name is required for GET operation\n";
            return 1;
        }
    }

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Failed to create socket" << endl;
        return 1;
    }

    // Set up server address
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP.c_str(), &addr.sin_addr) <= 0) {
        cerr << "Invalid address" << endl;
        close(sock);
        return 1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "Connection failed" << endl;
        close(sock);
        return 1;
    }

    // Create a request
    ClientRequest req;
    req.req_id = "1";
    req.req_type = operation == "GET" ? "read" : "write";
    req.file_path = file_name;

    // Record send time
    long long send_time = now_ms();

    if (operation == "PUT") {
        try {
            // Open file for reading in binary mode
            ifstream file(file_path, ios::binary);
            if (!file) {
                throw runtime_error("Cannot open file: " + file_path);
            }

            // First send the request header
            req.write_data = "";  // Initially empty, we'll send file content separately
            string json_line = req.convert_to_json() + "\n";
            cout << "Sending request header: " << json_line << endl;

            if (!send_all(sock, json_line)) {
                cerr << "Failed to send request header" << endl;
                close(sock);
                return 1;
            }

            // Send file contents in 1024-byte chunks
            const int CHUNK_SIZE = 1024;
            char buffer[CHUNK_SIZE];
            size_t total_bytes = 0;
            int chunk_count = 0;

            while (file) {
                file.read(buffer, CHUNK_SIZE);
                size_t bytes_read = file.gcount();
                if (bytes_read == 0) break;

                // If this is a partial chunk (less than 1024 bytes), pad with nulls
                if (bytes_read < CHUNK_SIZE) {
                    memset(buffer + bytes_read, '\0', CHUNK_SIZE - bytes_read);
                }

                if (!send_all(sock, buffer, CHUNK_SIZE)) {
                    cerr << "Failed to send file chunk" << endl;
                    close(sock);
                    return 1;
                }

                total_bytes += bytes_read;
                chunk_count++;
                cout << "Sent chunk " << chunk_count << " (" << bytes_read << " bytes)" << endl;
            }

            cout << "Finished sending file. Total bytes: " << total_bytes 
                 << " in " << chunk_count << " chunks" << endl;

        } catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
            close(sock);
            return 1;
        }
    } else {
        // For GET requests, just send the request header
        string json_line = req.convert_to_json() + "\n";
        cout << "Sending request: " << json_line << endl;

        if (!send_all(sock, json_line)) {
            cerr << "Failed to send request" << endl;
            close(sock);
            return 1;
        }
    }

    cout << "Request sent successfully" << endl;

    // Process response packets as they arrive
    cout << "\nWaiting for server response..." << endl;
    process_server_response(sock);
    long long receive_time = now_ms();

    cout << "\nTiming Information:" << endl;
    cout << "Send Time: " << send_time << " ms" << endl;
    cout << "Receive Time: " << receive_time << " ms" << endl;
    cout << "Round Trip Time: " << (receive_time - send_time) << " ms" << endl;

    close(sock);
    return 0;
}
