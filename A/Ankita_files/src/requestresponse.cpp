#include "requestresponse.h"
#include <iostream>
using namespace rapidjson;
using namespace std;

string RequestResponse::convert_to_json() const {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("req_id"); writer.Int(req_id);
    writer.Key("req_type"); writer.String(req_type.c_str());
    writer.Key("file_path"); writer.String(file_path.c_str());
    writer.Key("write_data"); writer.String(write_data.c_str());
    writer.Key("read_data"); writer.String(read_data.c_str());
    writer.Key("req_status"); writer.Bool(req_status);
    writer.Key("arrival_time"); writer.Int64(arrival_time);
    writer.Key("service_start_time"); writer.Int64(service_start_time);
    writer.Key("completion_time"); writer.Int64(completion_time);
    writer.EndObject();
    return buffer.GetString();
}

void RequestResponse::convert_from_json(const string &json) {
    Document doc;
    doc.Parse(json.c_str());
    if (!doc.IsObject()) {
        cerr << "JSON parse failed\n";
        return;
    }

    if (doc.HasMember("req_id")) req_id = doc["req_id"].GetInt();
    if (doc.HasMember("req_type")) req_type = doc["req_type"].GetString();
    if (doc.HasMember("file_path")) file_path = doc["file_path"].GetString();
    if (doc.HasMember("write_data")) write_data = doc["write_data"].GetString();
    if (doc.HasMember("read_data")) read_data = doc["read_data"].GetString();
    if (doc.HasMember("req_status")) req_status = doc["req_status"].GetBool();
    if (doc.HasMember("arrival_time")) arrival_time = doc["arrival_time"].GetInt64();
    if (doc.HasMember("service_start_time")) service_start_time = doc["service_start_time"].GetInt64();
    if (doc.HasMember("completion_time")) completion_time = doc["completion_time"].GetInt64();
}

