#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <chrono>
#include <cerrno>
#include <cstring>
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

bool send_all(int sock, const string &s) {
    const char* buf = s.c_str();
    size_t len = s.size();
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(sock, buf + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += (size_t)n;
    }
    return true;
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

int main() {
    // Server details
    string SERVER_IP = "127.0.0.1";
    int SERVER_PORT = 9000;

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
    req.req_type = "read";
    req.file_name = "test.txt";

    // Record send time
    long long send_time = now_ms();

    // Convert request to JSON and send
    string json_line = req.convert_to_json() + "\n";
    cout << "Sending request: " << json_line << endl;

    if (!send_all(sock, json_line)) {
        cerr << "Failed to send request" << endl;
        close(sock);
        return 1;
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
