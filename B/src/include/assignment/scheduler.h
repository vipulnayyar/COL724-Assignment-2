
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <map>
#include <chrono>
#include<iostream>
#include <netinet/tcp.h> 

using namespace std;

using namespace std::chrono;

#include <assignment/requestresponse.h>

#include <assignment/server-worker.h>

map<string, pair<int, long long>> file_lines;
mutex mutex_file_lines; 


class Scheduler{
    public:

    queue<ServerTask> fcfs_queue;

    int SEND_PACKET_SIZE = 1024;


    mutex mutex_performance;
    long long connections_processed = 0;
    float time_spent_on_server = 0;
    long long window_start_time = 0;


    long long window_size_ms = 5000;
    
    
    int packet_size;
    bool exiting = false;
    
    mutex mutex_queue;
    condition_variable cv;
    vector<thread> worker_threads;
    int worker_thread_count;
    int quantum_size = 0;

    string file_store_dir = "";

    virtual void push(ServerTask req) = 0;
    virtual ServerTask pop() = 0;

    virtual int get_queue_length() = 0;

    Scheduler(int server_threads, string file_dir, int p, int q){
        worker_thread_count = server_threads;
        packet_size = p;
        quantum_size = q;
        file_store_dir = file_dir;
    }

    long long get_current_timestamp(){
        auto timestamp_now = system_clock::now();               
        auto timestamp_ms = duration_cast<milliseconds>(         
            timestamp_now.time_since_epoch()
        ).count();

        return timestamp_ms;
    }

    void server_loop(string server_addr, int port){

        cout<<"Starting Server on "<<server_addr << " at port "<< port<<endl;
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);

        if(server_socket <= 0){
            cerr << "Socket Creation Failed"<<endl;
            exit(1);
        }

        sockaddr_in socket_address;
        socket_address.sin_family = AF_INET;
        socket_address.sin_addr.s_addr = inet_addr(server_addr.c_str());
        socket_address.sin_port = htons(port);

        int opt = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            cerr << "setsockopt Failed " << endl;
            return;
        }

        int enable = 1;
        if (setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&enable, sizeof(enable)) < 0) {
            std::cerr << "Setting setsockopt(TCP_NODELAY) failed" << std::endl;
        } else {
            cout << "Setting TCP_NODELAY as enabled." << std::endl;
        }

        int bind_status = ::bind(server_socket, (struct sockaddr*)&socket_address, sizeof(sockaddr));

        cout<<"Bind status = "<<bind_status <<endl;

        int listen_status = listen(server_socket, 10);

        cout<<"Listen status = "<<listen_status <<endl;


        window_start_time = get_current_timestamp();
        connections_processed = 0;

        for(int i = 0; i < worker_thread_count; ++i)
            worker_threads.emplace_back(&Scheduler::worker_loop, this, i);
        

       
        // connection loop
        while(1){


            sleep(2);

            cout<<"\nWaiting for new connection"<<endl;

            int addrlen = sizeof(socket_address);
            int conn = accept(server_socket, (struct sockaddr*)&socket_address, (socklen_t*)&addrlen);

            // Creating a new task for the new connection
            ServerTask task(conn, file_store_dir);
            task.internal_arrival_time = get_current_timestamp();


            char msg_buffer[1024] = {0};
            auto bytesRead = read(conn, msg_buffer, sizeof(msg_buffer));

            if(bytesRead < 0){
                cerr << "Error reading from client"<<endl;
                continue;
            }else if(bytesRead == 0){
                cerr << "Connection closed by client"<<endl;
                continue;
            }

            cout<<"Message buffer: "<<msg_buffer<<endl;

            task.convert_from_json(msg_buffer);

            task.display();

            if(task.req_type == "health_check"){
                handle_health_check(task);
                continue;

            }

            
            
            cout<<"Request header read from client"<<endl;

            push(task);

            cout<<"Request dispatched to queue";
    
  
        }
    }

    bool packet_send(int connection, string data){
        if(data.length() < SEND_PACKET_SIZE){
            int padding = SEND_PACKET_SIZE - data.length();
            data += string(padding, '\0');
        }
        
        cout<<"\nSending packet to client : "<<data<<endl;
        int sent = send(connection, data.c_str(), data.length(), 0);
        if(sent < 0){
            cerr << "Error sending data: " << strerror(errno) << endl;
            return false;
        }
        return true;

    }

    void handle_health_check(ServerTask task){
        
        HealthCheckResponse healthcheck_response;
        healthcheck_response.req_id = task.req_id;
        healthcheck_response.req_type = task.req_type;

        healthcheck_response.queue_length = get_queue_length();

        mutex_performance.lock();
        if(get_current_timestamp() - window_start_time >= window_size_ms){
            connections_processed = 0;
            time_spent_on_server = 0;
            window_start_time = get_current_timestamp();
        }
        
        // Avoid division by zero
        if(connections_processed > 0) {
            healthcheck_response.connection_throughput = (float)connections_processed / ((float)window_size_ms / 1000.0);
            healthcheck_response.average_time_spent_on_server = ((float)time_spent_on_server / (float)connections_processed);
        } else {
            healthcheck_response.connection_throughput = 0.0;
            healthcheck_response.average_time_spent_on_server = 0.0;
        }

        mutex_performance.unlock();

        healthcheck_response.response_status = true;

        cout<<"Healthcheck response: "<<healthcheck_response.convert_to_json()<<endl;

        string healthcheck_response_json = healthcheck_response.convert_to_json();
        if(!packet_send(task.internal_conn, healthcheck_response_json)) {
            cerr << "Failed to send complete healthcheck response" << endl;
        }

        close(task.internal_conn);

    }

    void worker_loop(int id){

        cout<<"\nStarting new Worker thread with id : "<<id << endl;
        ServerWorker server_worker(id, packet_size, quantum_size);

        ofstream metrics_file("server_worker_log.txt."+to_string(id));
        if(!metrics_file.is_open()){
            cerr << "Unable to open log file"<<endl;
            exit(1);
        }
        


        while(1){

            cout<<"\nWorker "<<id<<" trying to pop request from queue"<<endl;
            ServerTask task = pop();

            if(task.internal_service_start_time == 0)
                task.internal_service_start_time = get_current_timestamp();            
            
            cout<<"Request picked from queue by worker "<< id;

            

            
            ServerTask processed_task = server_worker.process_req(task);

            if (processed_task.task_status == "pending"){

                // Reinserting req at end of queue if original req is received

                cout<<"\nAdding request to back of queue"<<endl;
                push(processed_task);

            }

            if(processed_task.task_status == "error"){
                cerr << "Error processing request"<<endl;
                continue;
            }

            task.internal_completion_time = get_current_timestamp();

            metrics_file<< get_current_timestamp() <<","<<task.req_id<<","<<task.req_type<<","<<task.file_path<<","<<task.internal_arrival_time<<","<<task.internal_service_start_time<<","<<task.internal_completion_time<<endl;


            mutex_performance.lock();
            
            if(get_current_timestamp() - window_start_time >= window_size_ms){

                time_spent_on_server = 0;
                connections_processed = 0;
                window_start_time = get_current_timestamp();
            }

            connections_processed += 1;
            time_spent_on_server += (float)(task.internal_completion_time - task.internal_arrival_time);

            mutex_performance.unlock();

            /* if(processed_task.req_type=="write"){
                mutex_file_lines.lock();
                file_lines[processed_task.file_path] = {processed_task.write_num_lines, get_current_timestamp()};
                mutex_file_lines.unlock();
            } */

        }

        metrics_file.close();

    }

    void stop(){

        exiting = true;
        cv.notify_all();
        for (auto &t : worker_threads)
            t.join();
    }

    


};


class FCFSScheduler : public Scheduler{
    using Scheduler::Scheduler;
    public:

    


    void push(ServerTask task) override{
        mutex_queue.lock();

        fcfs_queue.push(task);

        mutex_queue.unlock();

        cv.notify_one();

    }
    

    ServerTask pop() override{

        unique_lock<std::mutex> lock(mutex_queue);

        cv.wait(lock, [this]{ return !fcfs_queue.empty() || exiting ; });
        
        if(!fcfs_queue.empty()){

            ServerTask task = fcfs_queue.front();
            fcfs_queue.pop();
            lock.unlock();

            return task;


        }
        lock.unlock();

        ServerTask task(0, "");
        return task;
    }

    int get_queue_length() override{
        return fcfs_queue.size();
    }

    


   
    
    
};

class CompareSJF {
    public:
    bool operator()(ServerTask a,  ServerTask b) {
        
        return a.internal_arrival_time > b.internal_arrival_time;

        // Fixme
        /* int a_num_lines_to_process = 0;
        int b_num_lines_to_process = 0;

        if (a.req_type == "write")
            a_num_lines_to_process = a.write_num_lines;
        else{
            if (file_lines.count(a.file_path) > 0)
                a_num_lines_to_process = file_lines[a.file_path].first;
            else{
                cout<<"\n Num files not known for path "<<a.file_path;
                exit(1);
            }
        }


        if (b.req_type == "write"){
            b_num_lines_to_process = b.write_num_lines;
        }else{
            if (file_lines.count(b.file_path) > 0)
                b_num_lines_to_process = file_lines[b.file_path].first;
            else{
                cout<<"\n Num files not known for path "<<b.file_path;
                exit(1);
            }
        }  

        if (a_num_lines_to_process != b_num_lines_to_process)
            return a_num_lines_to_process > b_num_lines_to_process;

        return a.internal_arrival_time > b.internal_arrival_time;
 */
    };

};


class SJFScheduler : public Scheduler{
    using Scheduler::Scheduler;
    public:

    priority_queue<ServerTask, vector<ServerTask>, CompareSJF> sjf_pq;
    
   

    void push(ServerTask req) override{
        mutex_queue.lock();

        sjf_pq.push(req);

        mutex_queue.unlock();

        cv.notify_one();

    }

    ServerTask pop() override {

        unique_lock<std::mutex> lock(mutex_queue);

        cv.wait(lock, [this]{ return !sjf_pq.empty() || exiting ; });
        
        if(!sjf_pq.empty()){

            ServerTask task = sjf_pq.top();
            sjf_pq.pop();
            lock.unlock();

            return task;


        }
        lock.unlock();

        ServerTask task(0, "");
        return task;

    }

    int get_queue_length() override{
        return sjf_pq.size();
    }

    
};

class RRScheduler : public Scheduler{
    using Scheduler::Scheduler;
    public:

    void push(ServerTask task) override{
        mutex_queue.lock();

        fcfs_queue.push(task);

        mutex_queue.unlock();

        cv.notify_one();

    }

    ServerTask pop() override{

        unique_lock<std::mutex> lock(mutex_queue);

        cv.wait(lock, [this]{ return !fcfs_queue.empty() || exiting ; });
        
        if(!fcfs_queue.empty()){

            ServerTask task = fcfs_queue.front();
            fcfs_queue.pop();
            lock.unlock();

            return task;


        }
        lock.unlock();

        ServerTask task(0, "");
        return task;
    }

    int get_queue_length() override{
        return fcfs_queue.size();
    }

    
};

