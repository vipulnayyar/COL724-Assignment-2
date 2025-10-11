#pragma once
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
using namespace std;

class RequestResponse {
public:
    int req_id;
    std::string req_type;
    std::string file_path;
    std::string write_data;
    std::string read_data;
    bool req_status;
    int read_line_index;
    int read_num_lines;
    long arrival_time;
    long service_start_time;
    long completion_time;

    std::string convert_to_json() const;
    void convert_from_json(const std::string &json);
};

