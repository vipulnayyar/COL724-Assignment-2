#pragma once
#include "requestresponse.h"

class ServerWorker {
public:
    int connection;
    RequestResponse request;
    int packet_size;
    int worker_id;

    ServerWorker(int id, int p);
    void process_req(RequestResponse req);
    void read();
    void write();
};

