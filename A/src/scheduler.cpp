
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
using namespace std::chrono;


map<string, pair<int, long long>> file_lines;
mutex mutex_file_lines; 

#include "server-worker.cpp"


class Scheduler{
    public:

    queue<RequestResponse> fcfs_queue;

    
    int packet_size;
    bool exiting = false;
    
    mutex mutex_queue;
    condition_variable cv;
    vector<thread> worker_threads;
    int worker_thread_count;
    int quantum_size = 0;

    string file_store_dir = "";

    virtual void push(RequestResponse req) = 0;
    virtual RequestResponse pop() = 0;

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

        int bind_status = ::bind(server_socket, (struct sockaddr*)&socket_address, sizeof(sockaddr));

        cout<<"Bind status = "<<bind_status <<endl;

        int listen_status = listen(server_socket, 10);

        cout<<"Listen status = "<<listen_status <<endl;

        for(int i = 0; i < worker_thread_count; ++i)
            worker_threads.emplace_back(&Scheduler::worker_loop, this, i);
        

       
        // connection loop
        while(1){


            cout<<"\nWaiting for new connection"<<endl;

            int addrlen = sizeof(socket_address);
            int conn = accept(server_socket, (struct sockaddr*)&socket_address, (socklen_t*)&addrlen);
            

            char msg_buffer[1000] = {0};
            auto bytesRead = read(conn, msg_buffer, 1000);
            //cout << "The message was: " << msg_buffer<<endl;

           

            
            RequestResponse req;
            
            req.convert_from_json(msg_buffer);
            req.arrival_time = get_current_timestamp();
            req.conn = conn;
            cout<<"Request received"<<endl;
            cout<<req.convert_to_json()<<endl;

            req.file_path = file_store_dir + "/" + req.file_path;

            push(req);

            cout<<"Request dispatched to queue";
    
  
        }
    }


     void worker_loop(int id){

        cout<<"\nStarting new Worker thread with id : "<<id << endl;
        ServerWorker server_worker(id, packet_size);

        while(1){

            RequestResponse req = pop();

            if(req.req_type == "none")
                continue;

            
            req.service_start_time = get_current_timestamp();

            
            
            cout<<"Request picked from queue by worker "<< id;

            RequestResponse processed_req = server_worker.process_req(req, quantum_size);

            if (processed_req.req_type == req.req_type){

                // Reinserting req at end of queue if original req is received

                cout<<"\nAdding request to back of queue"<<endl;
                push(processed_req);

            }

            if(req.req_type=="write"){
                mutex_file_lines.lock();
                file_lines[req.file_path] = {req.write_num_lines, get_current_timestamp()};
                mutex_file_lines.unlock();
            }

        }

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

    


    void push(RequestResponse req) override{
        mutex_queue.lock();

        fcfs_queue.push(req);

        mutex_queue.unlock();

        cv.notify_one();

    }
    

    RequestResponse pop() override{

        unique_lock<std::mutex> lock(mutex_queue);

        cv.wait(lock, [this]{ return !fcfs_queue.empty() || exiting ; });
        
        if(!fcfs_queue.empty()){

            RequestResponse req = fcfs_queue.front();
            fcfs_queue.pop();
            lock.unlock();

            return req;


        }
        lock.unlock();

        RequestResponse req;
        return req;
    }

    


   
    
    
};

class CompareSJF {
    public:
    bool operator()(RequestResponse a,  RequestResponse b) {
        
        int a_num_lines_to_process = 0;
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

        return a.arrival_time > b.arrival_time;

    };

};


class SJFScheduler : public Scheduler{
    using Scheduler::Scheduler;
    public:

    priority_queue<RequestResponse, vector<RequestResponse>, CompareSJF> sjf_pq;
    
   

    void push(RequestResponse req) override{
        mutex_queue.lock();

        sjf_pq.push(req);

        mutex_queue.unlock();

        cv.notify_one();

    }

    RequestResponse pop() override {

        unique_lock<std::mutex> lock(mutex_queue);

        cv.wait(lock, [this]{ return !sjf_pq.empty() || exiting ; });
        
        if(!sjf_pq.empty()){

            RequestResponse req = sjf_pq.top();
            sjf_pq.pop();
            lock.unlock();

            return req;


        }
        lock.unlock();

        RequestResponse req;
        return req;

    }

    
};

class RRScheduler : public Scheduler{
    using Scheduler::Scheduler;
    public:

    void push(RequestResponse req) override{
        mutex_queue.lock();

        fcfs_queue.push(req);

        mutex_queue.unlock();

        cv.notify_one();

    }

    RequestResponse pop() override{

        unique_lock<std::mutex> lock(mutex_queue);

        cv.wait(lock, [this]{ return !fcfs_queue.empty() || exiting ; });
        
        if(!fcfs_queue.empty()){

            RequestResponse req = fcfs_queue.front();
            fcfs_queue.pop();
            lock.unlock();

            return req;


        }
        lock.unlock();

        RequestResponse req;
        return req;
    }

    
};

