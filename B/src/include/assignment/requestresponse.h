
#pragma once

#include <string>
#include <vector>
using namespace std;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;
#include <iostream>

// Combined Request format for Read and Writes
/* class RequestResponse{

    public:

    int req_id = -1;
    string req_type = "none";
    int conn = -1;

    long long arrival_time = 0;
    long long service_start_time = 0;
    long long completion_time = 0;

    int read_line_index = -1;
    int write_num_lines = -1;
    string read_data;

    string file_path = "";
    string write_data = "";

    bool req_status = false;

    string read_packet_to_send = "";
    streampos read_chkpt_position;
    bool read_checkpoint_present = false;
    int read_checkpoint_lines_read = 0;
    bool read_start_of_file = true;


    bool write_checkpoint_present = false;
    int write_checkpoint_bytes_written;

   

    void convert_from_json(const string& json_str){

        Document doc;
        doc.Parse(json_str.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            cerr << "Failure of JSON parsing"<<endl;
            //cerr<<json_str.c_str();
            //cerr <<"Server exiting";
            //exit(1);
        }

        if (doc.HasMember("req_id")) req_id = doc["req_id"].GetInt();
        if (doc.HasMember("req_type")) req_type = doc["req_type"].GetString();
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

    string convert_to_json(){

        Document doc;
        doc.SetObject();

        Document::AllocatorType& allocator = doc.GetAllocator();

        doc.AddMember("req_id", req_id, allocator);
        doc.AddMember("req_type", Value(req_type.c_str(), allocator), allocator);
        doc.AddMember("arrival_time", rapidjson::Value().SetInt64(arrival_time), allocator);
        doc.AddMember("service_start_time", rapidjson::Value().SetInt64(service_start_time), allocator);
        doc.AddMember("completion_time", rapidjson::Value().SetInt64(completion_time), allocator);
        doc.AddMember("read_line_index", read_line_index, allocator);
        doc.AddMember("write_num_lines", write_num_lines, allocator);
        doc.AddMember("req_status", req_status, allocator);


        doc.AddMember("write_data", Value(write_data.c_str(), allocator), allocator);
        doc.AddMember("file_path", Value(file_path.c_str(), allocator), allocator);


        doc.AddMember("read_data", Value(read_data.c_str(), allocator), allocator);
        /* Value arr(kArrayType);
        for (const auto& s : read_data) {
            arr.PushBack(Value(s.c_str(), allocator), allocator);
        }
        doc.AddMember("read_data", arr, allocator); 


        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);

        return buffer.GetString();


    }

}; */


// max 1024 bytes
class ClientRequest{

    public:
    string req_id = "";
    string req_type = "";
    string file_path = "";
    

    string convert_to_json(){
        Document doc;
        doc.SetObject();
        Document::AllocatorType& allocator = doc.GetAllocator();

        doc.AddMember("req_id", Value(req_id.c_str(), allocator), allocator);
        doc.AddMember("req_type", Value(req_type.c_str(), allocator), allocator);
        doc.AddMember("file_path", Value(file_path.c_str(), allocator), allocator);

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        return buffer.GetString();
    }
   
};


class ServerTask{
    public:

    string req_id = "";
    string req_type = "";
    string file_path = "";

    string task_status = "pending";

    int internal_conn = -1;
    string internal_data_store_dir = "";
    long long internal_arrival_time = 0;
    long long internal_service_start_time = 0;
    long long internal_completion_time = 0;


    string internal_read_packet_to_send = "";
    streampos internal_read_chkpt_position;
    bool internal_read_checkpoint_present = false;
    int internal_read_checkpoint_lines_read = 0;
    bool internal_read_start_of_file = true;


    bool internal_write_checkpoint_present = false;
    int internal_write_checkpoint_bytes_written;


    ServerTask(int connection, string data_store_dir){
        internal_conn = connection;
        internal_data_store_dir = data_store_dir;
    }

    bool convert_from_json(const string& json_str){

        try{

            Document doc;
            doc.Parse(json_str.c_str());
            if (doc.HasParseError() || !doc.IsObject()) {
                cerr << "Error parsing JSON: Invalid JSON format or not an object" << endl;
                return false;
            }

            if (doc.HasMember("req_id")) req_id = doc["req_id"].GetString();
            if (doc.HasMember("req_type")) req_type = doc["req_type"].GetString();
            if (doc.HasMember("file_path")) file_path = doc["file_path"].GetString();

            return true;

        }catch(const exception& e){
            cerr<<"Error converting from JSON: "<<e.what()<<endl;
            cerr<<"JSON string: "<<json_str<<endl;
            return false;
        }
        
        return true;
    }

    void display(){
        cout<<"Request ID: "<<req_id<<endl;
        cout<<"Request Type: "<<req_type<<endl;
        cout<<"File Path: "<<file_path<<endl;
        cout<<"Task Status: "<<task_status<<endl;
        cout<<"Internal Arrival Time: "<<internal_arrival_time<<endl;
        cout<<"Internal Service Start Time: "<<internal_service_start_time<<endl;
        cout<<"Internal Completion Time: "<<internal_completion_time<<endl;
        cout<<"Internal Read Checkpoint Present: "<<internal_read_checkpoint_present<<endl;
        cout<<"Internal Read Checkpoint Position: "<<internal_read_chkpt_position<<endl;
        cout<<"Internal Read Checkpoint Lines Read: "<<internal_read_checkpoint_lines_read<<endl;
        cout<<"Internal Read Start of File: "<<internal_read_start_of_file<<endl;
        cout<<"Internal Write Checkpoint Present: "<<internal_write_checkpoint_present<<endl;
        cout<<"Internal Write Checkpoint Bytes Written: "<<internal_write_checkpoint_bytes_written<<endl;
    }
};


/* // max 1024 bytes
class ServerResponseHeader{
    public:

    string req_id = "";
    string req_type = "";
    string file_path = "";

    string convert_to_json(){
        Document doc;
        doc.SetObject();
        Document::AllocatorType& allocator = doc.GetAllocator();

        doc.AddMember("req_id", Value(req_id.c_str(), allocator), allocator);
        doc.AddMember("req_type", Value(req_type.c_str(), allocator), allocator);
        doc.AddMember("file_path", Value(file_path.c_str(), allocator), allocator);

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        return buffer.GetString();
    }



};
 */

/* class HealthCheckRequestResponse{
    public:
    string req_type = "health_check";
    bool health_check_status = false;

    string convert_to_json(){
        Document doc;
        doc.SetObject();
        Document::AllocatorType& allocator = doc.GetAllocator();

        doc.AddMember("req_type", Value(req_type.c_str(), allocator), allocator);
        doc.AddMember("health_check_status", health_check_status, allocator);

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        return buffer.GetString();
    }

    bool convert_from_json(const string& json_str){
        Document doc;
        doc.Parse(json_str.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            return false;
        }

        if (doc.HasMember("req_type") && doc.HasMember("health_check_status")) {
            req_type = doc["req_type"].GetString();
            health_check_status = doc["health_check_status"].GetBool();
            return true;
        }
        else{
            return false;
        }

    }
};
 */



