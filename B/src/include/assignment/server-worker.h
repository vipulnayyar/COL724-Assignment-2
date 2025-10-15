
#include <fstream>
#include <algorithm>
#include <string>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assignment/requestresponse.h>

using namespace std;
using namespace std::chrono;


class ServerWorker{

    public:

    ServerTask task;
    int lines_per_packet;
    int worker_id;
    int quantum_size;
    int SEND_PACKET_SIZE = 20;

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

    ServerWorker(int id, int lp, int q_size) : 
        task(-1, ""),
        worker_id(id),
        lines_per_packet(lp),
        quantum_size(q_size) {
    }

    ServerTask process_req(ServerTask srv_task){
        task = srv_task;
        processing_start_time = get_current_timestamp();

        if(task.req_type == "read"){
            return read();
        }
        // else if(task.req_type=="write"){

        //     return write();
        // }

        ServerTask dummytask(0, "");
        return dummytask;

    }

    ServerTask read(){

        string file_path = task.internal_data_store_dir + "/" + task.file_path;

        task.display();

        ifstream infile(file_path);

        if(!infile.is_open()){
            cerr << "Unable to open file for reading"<<file_path<<endl;
        }


        

        if(quantum_size > 0 && task.internal_read_checkpoint_present){
            
            // Restore read checkpoint
            infile.seekg(task.internal_read_chkpt_position);

        }else {
            task.internal_read_packet_to_send = "";
            task.internal_read_checkpoint_lines_read = 0;

        }

        task.task_status = "processing";

        int quantum_check_count = 0;
        char c;

        cout<<"\nReading file : "<<file_path<<endl;

        while (infile.get(c)){


            task.internal_read_packet_to_send += c;
            

            if (c == '\n')
                task.internal_read_checkpoint_lines_read ++;


            // check if read data has exceeded packet size or lines per packet
            if(task.internal_read_packet_to_send.length() == SEND_PACKET_SIZE
                || task.internal_read_checkpoint_lines_read == lines_per_packet){

                packet_send(task.internal_read_packet_to_send);

                task.internal_read_packet_to_send = "";
                task.internal_read_checkpoint_lines_read = 0;


            }

            // quantum check
            quantum_check_count ++;
            if ( quantum_check_count >= 100 &&  quantum_complete() ){
                quantum_check_count = 0;
                task.internal_read_checkpoint_present = true;
                task.internal_read_chkpt_position = infile.tellg();

                infile.close();
                task.task_status = "pending";
                return task;

            }
        
        }

        if (task.internal_read_packet_to_send.length() > 0){

            task.internal_completion_time = get_current_timestamp();

            packet_send(task.internal_read_packet_to_send);

        }
        
        close(task.internal_conn);

        infile.close();

        task.task_status = "completed";

        return task;

    }
/* 
    ServerTask write(){

        size_t total_data_bytes = task.write_data.size();
        int chunk_size = 10;

        ofstream outfile(task.file_path, ios::app);

        if(task.write_checkpoint_present == false)
            task.write_checkpoint_bytes_written = 0;

        if (!outfile.is_open()) {
            cerr << "Unable to open file for writing" << task.file_path << endl;

            task.task_status = "error";

            return task;
        }

        while(task.write_checkpoint_bytes_written < total_data_bytes){

            size_t to_write;
            if(chunk_size < (total_data_bytes - task.write_checkpoint_bytes_written))
                to_write = chunk_size;
            else
                to_write = (total_data_bytes - task.write_checkpoint_bytes_written);

            outfile.write(task.write_data.data() + 
            task.write_checkpoint_bytes_written, to_write);

            task.write_checkpoint_bytes_written += to_write;


            // Creating write checkpoint in task
            if(quantum_size > 0 && get_duration(processing_start_time, get_current_timestamp()) >= quantum_size){
                task.write_checkpoint_present = true;
                task.task_status = "pending";

                outfile.close();
                return task;

            }


        }

        cout<<"\n File written to "<<task.file_path<<endl;

        
        outfile.close();
        
        
        



        task.write_data = "";

        task.completion_time = get_current_timestamp();

        string response_message = task.convert_to_json();
        cout<<"\n Sending server response to client : \n"<<response_message<<endl;
        send(task.internal_conn, response_message.c_str(), response_message.size(), 0);

        
        close(task.internal_conn);

        task.task_status = "completed";

        return task;


    }
 */

    void packet_send(string data){

        
        if(data.length() < SEND_PACKET_SIZE){
            int padding = SEND_PACKET_SIZE - data.length();
            data += string(padding, '\0');
        }
        cout<<"\nSending packet to client : "<<data<<endl;
        send(task.internal_conn, data.c_str(), data.length(), 0);

    }


    bool quantum_complete(){
        if(quantum_size > 0)
            if(get_duration(processing_start_time, get_current_timestamp()) >= quantum_size)
                return true;
        
        return false;
    }


};