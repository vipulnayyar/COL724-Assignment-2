
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "server-worker.cpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <map>
#include <chrono>
using namespace std::chrono;




class Scheduler{
    public:


    map<string, int> file_lines;
    mutex mutex_file_lines; 

    int packet_size;
    bool exiting = false;
    queue<RequestResponse> request_queue;
    mutex mutex_queue;
    condition_variable cv;
    vector<thread> worker_threads;
    int worker_thread_count;

    virtual void push(RequestResponse req) = 0;

    Scheduler(int server_threads, int p){
        worker_thread_count = server_threads;
        packet_size = p;
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

            auto timestamp_now = system_clock::now();               
            auto timestamp_ms = duration_cast<milliseconds>(         
                timestamp_now.time_since_epoch()
            ).count();

            
            RequestResponse req;
            
            req.convert_from_json(msg_buffer);
            req.arrival_time = timestamp_ms;
            req.conn = conn;
            cout<<"Request received"<<endl;
            cout<<req.convert_to_json()<<endl;

            push(req);

            cout<<"Request dispatched to queue";
    
  
        }
    }


     void worker_loop(int id){

        cout<<"\nStarting new Worker thread with id : "<<id << endl;
        ServerWorker server_worker(id, packet_size);

        while(1){

            unique_lock<std::mutex> lock(mutex_queue);

            cv.wait(lock, [this]{ return !request_queue.empty() || exiting ; });
         
            if(!request_queue.empty()){

                cout<<"Request picked from queue by worker "<< id;
                RequestResponse req = request_queue.front();
                request_queue.pop();
                lock.unlock();

                auto timestamp_now = system_clock::now();               
                auto timestamp_ms = duration_cast<milliseconds>(         
                    timestamp_now.time_since_epoch()
                ).count();

                req.service_start_time = timestamp_ms;

                server_worker.process_req(req);

                if(req.req_type=="write"){
                    mutex_file_lines.lock();
                    file_lines[req.file_path] = req.write_num_lines;
                    mutex_file_lines.unlock();
                }

            }

            // Add Clean exit

            
            

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

        request_queue.push(req);

        mutex_queue.unlock();

        cv.notify_one();

    }
    


    


   
    
    
};

class SJFScheduler : public Scheduler{
    using Scheduler::Scheduler;
    public:

    int worker_threads;
    int packet_size;

   

    void push(RequestResponse req) override{
        mutex_queue.lock();

        request_queue.push(req);

        mutex_queue.unlock();

        cv.notify_one();

    }

    
};

class RRScheduler : public Scheduler{
    public:

    int worker_threads;
    int packet_size;
    int quantum_size;

    RRScheduler(int server_threads, int p, int q ) 
    : Scheduler(server_threads, p), quantum_size(p){
        
    }

    void push(RequestResponse req) override{
        mutex_queue.lock();

        request_queue.push(req);

        mutex_queue.unlock();

        cv.notify_one();

    }

    
};

