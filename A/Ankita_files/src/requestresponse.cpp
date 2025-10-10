#include "requestresponse.h"
#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

RequestResponse::RequestResponse()
    : req_id(-1), req_type("none"),
      arrival_time(0), service_start_time(0), completion_time(0),
      read_line_index(-1), read_num_lines(-1),
      file_path(""), write_data(""),
      req_status(false) {}


void RequestResponse::convert_from_json(const string& json_str) {
    Document doc;
    doc.Parse(json_str.c_str());
    if (doc.HasParseError() || !doc.IsObject()) {
        cerr << "Failure of JSON parsing" << endl;
        throw runtime_error("Invalid JSON in convert_from_json");
    }

    if (doc.HasMember("req_id")) req_id = doc["req_id"].GetInt();
    if (doc.HasMember("req_type")) req_type = doc["req_type"].GetString();
    if (doc.HasMember("arrival_time")) arrival_time = doc["arrival_time"].GetInt();
    if (doc.HasMember("service_start_time")) service_start_time = doc["service_start_time"].GetInt();
    if (doc.HasMember("completion_time")) completion_time = doc["completion_time"].GetInt();
    if (doc.HasMember("read_line_index")) read_line_index = doc["read_line_index"].GetInt();
    if (doc.HasMember("read_num_lines")) read_num_lines = doc["read_num_lines"].GetInt();

    if (doc.HasMember("write_data")) write_data = doc["write_data"].GetString();
    if (doc.HasMember("file_path")) file_path = doc["file_path"].GetString();

    if (doc.HasMember("req_status")) req_status = doc["req_status"].GetBool();

    if (doc.HasMember("read_data")) read_data = doc["read_data"].GetString();
}


string RequestResponse::convert_to_json() {
    Document doc;
    doc.SetObject();

    Document::AllocatorType& allocator = doc.GetAllocator();

    doc.AddMember("req_id", req_id, allocator);
    doc.AddMember("req_type", Value(req_type.c_str(), allocator), allocator);
    doc.AddMember("arrival_time", arrival_time, allocator);
    doc.AddMember("service_start_time", service_start_time, allocator);
    doc.AddMember("completion_time", completion_time, allocator);
    doc.AddMember("read_line_index", read_line_index, allocator);
    doc.AddMember("read_num_lines", read_num_lines, allocator);
    doc.AddMember("req_status", req_status, allocator);

    doc.AddMember("write_data", Value(write_data.c_str(), allocator), allocator);
    doc.AddMember("file_path", Value(file_path.c_str(), allocator), allocator);
    doc.AddMember("read_data", Value(read_data.c_str(), allocator), allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

