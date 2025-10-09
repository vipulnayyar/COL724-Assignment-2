#include <string>
#include <vector>
using namespace std;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

// Combined Request format for Read and Writes
class RequestResponse{

    public:

    int req_id = -1;
    string req_type = "none";

    int arrival_time = 0;
    int service_start_time = 0;
    int completion_time = 0;

    int read_line_index = -1;
    int read_num_lines = -1;
    string read_data;

    string file_path = "";
    string write_data = "";

    bool req_status = false;

    /* void append_line_data(string line){
        read_data.push_back(line);
    } */


    void convert_from_json(const string& json_str){

        Document doc;
        doc.Parse(json_str.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            cerr << "Failure of JSON parsing"<<endl;
            exit(1);
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

    string convert_to_json(){

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
        /* Value arr(kArrayType);
        for (const auto& s : read_data) {
            arr.PushBack(Value(s.c_str(), allocator), allocator);
        }
        doc.AddMember("read_data", arr, allocator); */


        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);

        return buffer.GetString();


    }

};