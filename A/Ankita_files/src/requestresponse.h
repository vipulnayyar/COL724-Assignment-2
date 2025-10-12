#pragma once
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

class RequestResponse {
public:
    int req_id = -1;
    std::string req_type = "none";
    int conn = -1;                 // needed for server socket
    std::string file_path;
    std::string write_data;
    std::string read_data;
    bool req_status = false;
    int read_line_index = -1;
    int read_num_lines = -1;
    int write_num_lines = -1;      // needed for Scheduler
    long long arrival_time = 0;
    long long service_start_time = 0;
    long long completion_time = 0;

    void convert_from_json(const std::string &json_str);
    std::string convert_to_json() const;
};

