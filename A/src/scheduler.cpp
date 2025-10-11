
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "server-worker.cpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>


class Scheduler{
    public:

    

};


class FCFSScheduler : public Scheduler{
    public:

    int packet_size;
    bool exiting = false;
    queue<RequestResponse> request_queue;
    mutex mutex_queue;
    condition_variable cv;
    vector<thread> worker_threads;

    FCFSScheduler(int server_threads, int p){
        packet_size = p;

        for(int i = 0; i < server_threads; ++i)
            worker_threads.emplace_back(&FCFSScheduler::worker_loop, this, i);

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

        

       
       
        // connection loop
        while(1){


            cout<<"Waiting for new connection"<<endl;

            int addrlen = sizeof(socket_address);
            int conn = accept(server_socket, (struct sockaddr*)&socket_address, (socklen_t*)&addrlen);
            

            char msg_buffer[1000] = {0};
            auto bytesRead = read(conn, msg_buffer, 1000);
            cout << "The message was: " << msg_buffer<<endl;

            RequestResponse req;
            req.convert_from_json(msg_buffer);
            req.conn = conn;
            cout<<req.convert_to_json()<<endl;

            push(req);

            
  
        }
    }

    void push(RequestResponse req){
        mutex_queue.lock();

        request_queue.push(req);

        mutex_queue.unlock();

        cv.notify_one();

    }


    void worker_loop(int id){
        ServerWorker server_worker(packet_size);

        while(1){

            unique_lock<std::mutex> lock(mutex_queue);

            cv.wait(lock, [this]{ return !request_queue.empty() || exiting ; });
         
            if(!request_queue.empty()){
                RequestResponse req = request_queue.front();
                request_queue.pop();
                lock.unlock();
                server_worker.process_req(req);

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

class SJFScheduler : public Scheduler{
    public:

    int worker_threads;
    int packet_size;

    SJFScheduler(int server_threads, int p){
        worker_threads = server_threads;
        packet_size = p;
    }

    void server_loop(){

        while(1){
            
        }
    }

    
};

class RRScheduler : public Scheduler{
    public:

    int worker_threads;
    int packet_size;
    int quantum_size;

    RRScheduler(int server_threads, int p, int q ){
        worker_threads = server_threads;
        packet_size = p;
        quantum_size = q;
    }

    void server_loop(){

        while(1){
            
        }
    }

    
};

