#include "requestresponse.h"
#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

void RequestResponse::convert_from_json(const std::string& json_str){
    Document doc;
    doc.Parse(json_str.c_str());
    if (doc.HasParseError() || !doc.IsObject()) {
        cerr << "Failure of JSON parsing" << endl;
        exit(1);
    }

    if (doc.HasMember("req_id")) req_id = doc["req_id"].GetInt();
    if (doc.HasMember("req_type")) req_type = doc["req_type"].GetString();
    if (doc.HasMember("conn")) conn = doc["conn"].GetInt();
    if (doc.HasMember("arrival_time")) arrival_time = doc["arrival_time"].GetInt64();
    if (doc.HasMember("service_start_time")) service_start_time = doc["service_start_time"].GetInt64();
    if (doc.HasMember("completion_time")) completion_time = doc["completion_time"].GetInt64();
    if (doc.HasMember("read_line_index")) read_line_index = doc["read_line_index"].GetInt();
    if (doc.HasMember("write_num_lines")) write_num_lines = doc["write_num_lines"].GetInt();
    if (doc.HasMember("write_data")) write_data = doc["write_data"].GetString();
    if (doc.HasMember("file_path")) file_path = doc["file_path"].GetString();
    if (doc.HasMember("req_status")) req_status = doc["req_status"].GetBool();
    if (doc.HasMember("read_data")) read_data = doc["read_data"].GetString();
}

std::string RequestResponse::convert_to_json() const {
    Document doc;
    doc.SetObject();
    Document::AllocatorType& allocator = doc.GetAllocator();

    doc.AddMember("req_id", req_id, allocator);
    doc.AddMember("req_type", Value(req_type.c_str(), allocator), allocator);
    doc.AddMember("conn", conn, allocator);
    doc.AddMember("arrival_time", static_cast<int64_t>(arrival_time), allocator);
    doc.AddMember("service_start_time", static_cast<int64_t>(service_start_time), allocator);
    doc.AddMember("completion_time", static_cast<int64_t>(completion_time), allocator);
    doc.AddMember("read_line_index", read_line_index, allocator);
    doc.AddMember("write_num_lines", write_num_lines, allocator);
    doc.AddMember("req_status", req_status, allocator);
    doc.AddMember("write_data", Value(write_data.c_str(), allocator), allocator);
    doc.AddMember("file_path", Value(file_path.c_str(), allocator), allocator);
    doc.AddMember("read_data", Value(read_data.c_str(), allocator), allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

