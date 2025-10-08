#include <string>
using namespace std;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

// Combined Request format for Read and Writes
class Request{

    public:

    int req_id = -1;
    string req_type = "none";

    int arrival_time = 0;
    int service_start_time = 0;
    int completion_time = 0;

    int read_line_index = -1;

    string write_data = "";

    Request(int id, string type){
        req_id = id;
        req_type = type;
    }


    void set_read_line_index(int read_index){
        read_line_index = read_index;
    }


    void set_write_data(string data){
        write_data = data;
    }

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
        if (doc.HasMember("write_data")) write_data = doc["write_data"].GetString();


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
        doc.AddMember("write_data", Value(write_data.c_str(), allocator), allocator);


        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);

        return buffer.GetString();


    }

};