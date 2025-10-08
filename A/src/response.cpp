#include <string>
#include <vector>
using namespace std;


class Response{

    public:

    int req_id = -1;
    string req_type = "none";

    int arrival_time = 0;
    int service_start_time = 0;
    int completion_time = 0;

    int read_line_index = -1;


    int next_line_to_query = -1;

    vector<string> read_data;

    bool req_status = false;

    Response(int id, string type){
        req_id = id;
        req_type = type;
    }


    void set_read_line_index(int read_index){
        read_line_index = read_index;
    }

    void set_arrival_time(int t){
        arrival_time = t;
    }

    void set_service_start_time(int t){
        service_start_time = t;
    }

    void set_completion_time(int t){
        completion_time = t;
    }

    void set_next_line_to_query(int d){
        next_line_to_query = d;
    }

    void append_line_data(string line){
        read_data.push_back(line);
    }

    void set_req_status(bool s){
        req_status = s;
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
        doc.AddMember("next_line_to_query", next_line_to_query, allocator);
        doc.AddMember("req_status", req_status, allocator);

        Value arr(kArrayType);
        for (const auto& s : read_data) {
            arr.PushBack(Value(s.c_str(), allocator), allocator);
        }
        doc.AddMember("read_data", arr, allocator);


        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);

        return buffer.GetString();


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
        if (doc.HasMember("next_line_to_query")) next_line_to_query = doc["next_line_to_query"].GetInt();

        if (doc.HasMember("req_status")) req_status = doc["req_status"].GetBool();

        if (doc.HasMember("read_data") && doc["read_data"].IsArray()) {
            read_data.clear();
            for (auto& v : doc["read_data"].GetArray()) {
                read_data.push_back(v.GetString());
            }
        }


    }

};