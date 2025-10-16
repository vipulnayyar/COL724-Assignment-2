
#pragma once

#include <string>
#include <vector>
using namespace std;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;
#include <iostream>


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

class ServerResponse{
    public:
    string req_id = "";
    string req_type = "";
    string file_path = "";
    bool response_status = false;
    
    
    string convert_to_json(){
        Document doc;
        doc.SetObject();
        Document::AllocatorType& allocator = doc.GetAllocator();

        doc.AddMember("req_id", Value(req_id.c_str(), allocator), allocator);
        doc.AddMember("req_type", Value(req_type.c_str(), allocator), allocator);
        doc.AddMember("file_path", Value(file_path.c_str(), allocator), allocator);
        doc.AddMember("response_status", Value(response_status), allocator);

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


    bool internal_write_checkpoint_present = false;


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

        cout<<"Internal Write Checkpoint Present: "<<internal_write_checkpoint_present<<endl;
    }
};



