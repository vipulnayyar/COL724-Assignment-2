
#include <fstream>
#include "requestresponse.cpp"
#include "file-io.cpp"

class ServerWorker{

    public:

    int connection;
    RequestResponse request;
    int packet_size;


    ServerWorker(int p){
        packet_size = p;

    }

    void process_req( int conn, RequestResponse req){

        connection = conn;
        request = req;

        if(request.req_type == "read"){
            read();
        }
        else if(request.req_type=="write"){

            write();
        }

    }

    void read(){

        vector<RequestResponse> responses;



        for(int line = 1; line <= request.read_num_lines; line += packet_size ){

        
            string data = read_file(request.file_path, request.read_line_index, packet_size);
        
            request.req_status = true;
            request.read_data = data;


            string response_message = request.convert_to_json();
            send(connection, response_message.c_str(), response_message.size(), 0);

        }

        
    }

    void write(){

        
        write_file(request.file_path, request.write_data);

        request.write_data = "";
        request.req_status = true;


        string response_message = request.convert_to_json();
        send(connection, response_message.c_str(), response_message.size(), 0);

        




    }
};