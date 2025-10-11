#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>

#include "requestresponse.h"
#include "file-io.h"   // contains read_file() and write_file()

using namespace std;

class ServerWorker {
public:
    int connection;
    RequestResponse request;
    int packet_size;

    explicit ServerWorker(int p) : connection(-1), packet_size(p) {}

    void process_req(int conn, const RequestResponse &req) {
        connection = conn;
        request = req;

        if (request.req_type == "read") {
            handle_read();
        } else if (request.req_type == "write") {
            handle_write();
        } else {
            cerr << "[ERROR] Unknown request type: " << request.req_type << endl;
        }
    }

private:
    void handle_read() {
        int start_index = request.read_line_index;
        int total_lines = request.read_num_lines;

        cout << "[INFO] Handling READ for file: " << request.file_path
             << " | packet_size=" << packet_size << endl;

        // Loop and send file content in chunks
        while (true) {
            string data = read_file(request.file_path, start_index, packet_size);
            if (data.empty()) break; // EOF or no more data

            request.read_data = data;
            request.req_status = false; // intermediate packet
            request.arrival_time = 0;   // not needed on server replies
            request.service_start_time = 0;
            request.completion_time = 0;

            string response_message = request.convert_to_json() + "\n";
            send(connection, response_message.c_str(), response_message.size(), 0);

            start_index += packet_size;
            if ((int)data.size() < packet_size) break; // reached end
        }

        // Send final completion packet
        request.req_status = true;
        request.read_data = "";
        string final_message = request.convert_to_json() + "\n";
        send(connection, final_message.c_str(), final_message.size(), 0);
    }

    void handle_write() {
        cout << "[INFO] Handling WRITE for file: " << request.file_path << endl;

        write_file(request.file_path, request.write_data);

        request.write_data = "";
        request.req_status = true;

        string response_message = request.convert_to_json() + "\n";
        send(connection, response_message.c_str(), response_message.size(), 0);
    }
};

