
#include <fstream>
#include "requestresponse.cpp"
#include "file-io.cpp"
using namespace std::chrono;


class ServerWorker{

    public:

    int connection;
    RequestResponse request;
    int packet_size;
    int worker_id;
    int quantum_size;

    long long processing_start_time;


    long long get_current_timestamp(){
        auto timestamp_now = system_clock::now();               
        auto timestamp_ms = duration_cast<milliseconds>(         
            timestamp_now.time_since_epoch()
        ).count();

        return timestamp_ms;

    }

    long long get_duration(long long old_ts, long long new_ts ){
        return (new_ts - old_ts);
    }

    ServerWorker(int id, int p){
        worker_id = id;
        packet_size = p;

    }

    RequestResponse process_req( RequestResponse req, int q_size){

        connection = req.conn;
        request = req;
        quantum_size = q_size;

        processing_start_time = get_current_timestamp();

        if(request.req_type == "read"){
            return read();
        }
        else if(request.req_type=="write"){

            return write();
        }

        RequestResponse dummyreq;
        return dummyreq;

    }

    RequestResponse read(){

        ifstream infile(request.file_path);

        if(!infile.is_open()){
            cerr << "Unable to open file for reading"<<request.file_path<<endl;
        }


        cout<<"Reading file : "<<request.file_path<<endl;

        string line;
        //string packet_to_send = "";
        //int line_count = 0;

        if(request.read_checkpoint_present){
            
            // Check if file has been written after partially processing this read request
            
            if(request.service_start_time < file_lines[request.file_path].second){
                cout<<"\nFile modified between partial read. Closing read connection"<<endl;
                close(request.conn);

            }

            // Restore read checkpoint

            infile.seekg(request.read_chkpt_position);

        }else {
            request.read_packet_to_send = "";
            request.read_checkpoint_lines_read = 0;
        }
        
        while (getline(infile, line)) {
            request.read_packet_to_send += line;

            request.read_checkpoint_lines_read ++;

            if (request.read_checkpoint_lines_read >= packet_size){
                request.req_status = true;
                request.read_data = request.read_packet_to_send;

               
                request.completion_time = get_current_timestamp();

                string response_message = request.convert_to_json();

                cout<<"\n Sending server response to client : \n"<<response_message<<endl;
                send(request.conn, response_message.c_str(), response_message.size(), 0);

                request.read_checkpoint_lines_read = 0;
                request.read_packet_to_send = "";

            }

            // Creating read checkpoint in request
            if(get_duration(processing_start_time, get_current_timestamp()) >= quantum_size){
                request.read_checkpoint_present = true;
                request.read_chkpt_position = infile.tellg();

                return request;

            }

            

        }

        if (request.read_packet_to_send.length() > 0){
            request.req_status = true;
            request.read_data = request.read_packet_to_send;


            request.completion_time = get_current_timestamp();

            string response_message = request.convert_to_json();


            cout<<"\n Sending server response to client : \n"<<response_message<<endl;
            send(request.conn, response_message.c_str(), response_message.size(), 0);

        }
        
        close(request.conn);


        RequestResponse req;
        return req;

        
        
    }

    RequestResponse write(){

        
        write_file(request.file_path, request.write_data);

        request.write_data = "";
        request.req_status = true;

        auto timestamp_now = system_clock::now();               
        auto timestamp_ms = duration_cast<milliseconds>(         
            timestamp_now.time_since_epoch()
        ).count();

        request.completion_time = timestamp_ms;

        string response_message = request.convert_to_json();
        cout<<"\n Sending server response to client : \n"<<response_message<<endl;
        send(request.conn, response_message.c_str(), response_message.size(), 0);



        
        close(request.conn);

        RequestResponse req;
        return req;


    }
};