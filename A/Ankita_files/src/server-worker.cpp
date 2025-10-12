#include "server-worker.h"
#include "file-io.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
using namespace std;
using namespace std::chrono;

ServerWorker::ServerWorker(int id, int p) {
    worker_id = id;
    packet_size = p;
}

void ServerWorker::process_req(RequestResponse req){
    connection = req.conn;
    request = req;

    if(request.req_type == "read"){
        read();
    }
    else if(request.req_type=="write"){
        write();
    }
}

void ServerWorker::read() {
    ifstream infile(request.file_path);
    if(!infile.is_open()){
        cerr << "Unable to open file for reading " << request.file_path << endl;
        return;
    }

    string line, packet_to_send = "";
    int line_count = 0;

    while (getline(infile, line)) {
        packet_to_send += line;
        line_count++;

        if (line_count >= packet_size){
            request.req_status = true;
            request.read_data = packet_to_send;

            auto timestamp_now = system_clock::now();
            auto timestamp_ms = duration_cast<milliseconds>(
                timestamp_now.time_since_epoch()
            ).count();
            request.completion_time = timestamp_ms;

            string response_message = request.convert_to_json();
            cout << "\n Sending server response to client : \n" << response_message << endl;
            send(connection, response_message.c_str(), response_message.size(), 0);

            line_count = 0;
            packet_to_send = "";
        }
    }

    if (packet_to_send.length() > 0){
        request.req_status = true;
        request.read_data = packet_to_send;

        auto timestamp_now = system_clock::now();
        auto timestamp_ms = duration_cast<milliseconds>(
            timestamp_now.time_since_epoch()
        ).count();
        request.completion_time = timestamp_ms;

        string response_message = request.convert_to_json();
        cout << "\n Sending server response to client : \n" << response_message << endl;
        send(connection, response_message.c_str(), response_message.size(), 0);
    }
    // Server-side log
    cout << "================ Server Log ================\n";
    cout << "Request ID       : " << request.req_id << endl;
    cout << "Request Type     : " << request.req_type << endl;
    cout << "File Path        : " << request.file_path << endl;
    cout << "Arrival Time     : " << request.arrival_time << endl;
    cout << "Service Start    : " << request.service_start_time << endl;
    cout << "Completion Time  : " << request.completion_time << endl;
    cout << "Worker ID        : " << worker_id << endl;
    cout << "Request Status   : " << (request.req_status ? "success" : "fail") << endl;
    cout << "===========================================\n";


    close(connection);
}

void ServerWorker::write() {
    write_file(request.file_path, request.write_data);
    request.write_data = "";
    request.req_status = true;

    auto timestamp_now = system_clock::now();
    auto timestamp_ms = duration_cast<milliseconds>(
        timestamp_now.time_since_epoch()
    ).count();
    request.completion_time = timestamp_ms;

    string response_message = request.convert_to_json();
    cout << "\n Sending server response to client : \n" << response_message << endl;
    send(connection, response_message.c_str(), response_message.size(), 0);
    // Server-side log
    cout << "================ Server Log ================\n";
    cout << "Request ID       : " << request.req_id << endl;
    cout << "Request Type     : " << request.req_type << endl;
    cout << "File Path        : " << request.file_path << endl;
    cout << "Arrival Time     : " << request.arrival_time << endl;
    cout << "Service Start    : " << request.service_start_time << endl;
    cout << "Completion Time  : " << request.completion_time << endl;
    cout << "Worker ID        : " << worker_id << endl;
    cout << "Request Status   : " << (request.req_status ? "success" : "fail") << endl;
    cout << "===========================================\n";


    close(connection);
}

