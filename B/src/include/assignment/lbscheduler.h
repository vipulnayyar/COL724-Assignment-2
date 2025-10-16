#include <assignment/requestresponse.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <arpa/inet.h>
#include <fstream>
#include <vector>
using namespace std;
using namespace std::chrono;



class LBScheduler{
    public:

    string server1_ip = "";
    int server1_port = -1;
    string server2_ip = "";
    int server2_port = -1;
    string server3_ip = "";
    int server3_port = -1;
    string server4_ip = "";
    int server4_port = -1;

    string lb_ip = "";
    int lb_port = -1;

    int lb_downstream_socket = -1;

    string sched_policy = "rr";

    vector<thread> connection_threads;

    vector<thread> healthcheck_threads;


    long long get_current_timestamp(){
        auto timestamp_now = system_clock::now();               
        auto timestamp_ms = duration_cast<milliseconds>(         
            timestamp_now.time_since_epoch()
        ).count();

        return timestamp_ms;

    }

    LBScheduler(string srv1_ip, int srv1_port, string srv2_ip, int srv2_port, string srv3_ip, int srv3_port, string srv4_ip, int srv4_port, string load_balancer_ip, int load_balancer_port, string sched_policy){

        lb_downstream_socket = create_downstream_socket(lb_ip, lb_port);

        server1_ip = srv1_ip;
        server1_port = srv1_port;
        server2_ip = srv2_ip;
        server2_port = srv2_port;
        server3_ip = srv3_ip;
        server3_port = srv3_port;
        server4_ip = srv4_ip;
        server4_port = srv4_port;

        lb_ip = load_balancer_ip;
        lb_port = load_balancer_port;

        sched_policy = "rr";


        healthcheck_threads.emplace_back(&LBScheduler::handle_healthcheck_requests, this, server1_ip, server1_port);
        healthcheck_threads.emplace_back(&LBScheduler::handle_healthcheck_requests, this, server2_ip, server2_port);
        healthcheck_threads.emplace_back(&LBScheduler::handle_healthcheck_requests, this, server3_ip, server3_port);
        healthcheck_threads.emplace_back(&LBScheduler::handle_healthcheck_requests, this, server4_ip, server4_port);

    }



    void handle_connection_traffic(int source_connection, int destination_connection){

    
        char buffer[1024] = {0};

        while(1){

            int read_bytes = read(source_connection, buffer, 1024);
            if(read_bytes < 0){
                cerr << "Error reading from source connection"<<endl;
                break;
            }else if(read_bytes == 0){
                cerr << "Connection closed by source connection"<<endl;
                break;
            }

            int send_bytes = send(destination_connection, buffer, read_bytes, 0);
            if(send_bytes < 0){
                cerr << "Error sending to destination connection"<<endl;
                break;
            }else if(send_bytes == 0){
                cerr << "Connection closed by destination connection"<<endl;
                break;
            }
            else{
                cout<<"Data sent to destination connection"<<endl;
            }
        }


        close(source_connection);
        close(destination_connection);

    }





    int create_downstream_socket(string ip, int port){

        int downstream_socket = socket(AF_INET, SOCK_STREAM, 0);

        if(downstream_socket <= 0){
            cerr << "Socket Creation Failed"<<endl;
            exit(1);
        }

        sockaddr_in socket_address;
        socket_address.sin_family = AF_INET;
        socket_address.sin_addr.s_addr = inet_addr(ip.c_str());
        socket_address.sin_port = htons(port);

        int opt = 1;
        if (setsockopt(downstream_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            cerr << "setsockopt Failed " << endl;
            return -1;
        }

        int enable = 1;
        if (setsockopt(downstream_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&enable, sizeof(enable)) < 0) {
            std::cerr << "Setting setsockopt(TCP_NODELAY) failed" << std::endl;
        } else {
            cout << "Setting TCP_NODELAY as enabled." << std::endl;
        }

        int bind_status = ::bind(downstream_socket, (struct sockaddr*)&socket_address, sizeof(sockaddr));

        cout<<"Bind status = "<<bind_status <<endl;

        int listen_status = listen(downstream_socket, 10);

        cout<<"Listen status = "<<listen_status <<endl;

        return downstream_socket;
    }

    pair<string, int> schedule_unweighted_round_robin_server(){
        static int current_server_index = 0;
        current_server_index = (current_server_index) % 4;
        switch(current_server_index){
        case 0:
            return {server1_ip, server1_port};
        case 1:
            return {server2_ip, server2_port};
        case 2:
            return {server3_ip, server3_port};
        case 3:
            return {server4_ip, server4_port};
        default:
            return {server1_ip, server1_port};
       
        };

    }

    int schedule_upstream_socket(){
        if(sched_policy == "rr"){
            pair<string, int> server_info =  schedule_unweighted_round_robin_server();

            return create_upstream_socket(server_info.first, server_info.second);

        }else{

            return create_upstream_socket(server1_ip, server1_port);
        }
    }

    int create_upstream_socket(string ip, int port){


        cout<<"\nCreating upstream socket to server "<<ip << " at port "<< port <<endl;

        int upstream_socket = socket(AF_INET, SOCK_STREAM, 0);

        if(upstream_socket <= 0){
            cerr << "Socket Creation Failed"<<endl;
            exit(1);
        }

        sockaddr_in socket_address;
        socket_address.sin_family = AF_INET;
        socket_address.sin_addr.s_addr = inet_addr(ip.c_str());
        socket_address.sin_port = htons(port);

        int connect_status = connect(upstream_socket, (struct sockaddr*)&socket_address, sizeof(sockaddr));
        if(connect_status < 0){
            cerr << "Upstream Connection Failed"<<endl;
            exit(1);
        }

        cout<<"Upstream Connection from LB to Server "<<ip << " at port "<< port <<" Successful"<<endl;

        return upstream_socket;
    }

    int get_downstream_connection(){
        
        sockaddr_in socket_address;
        int addrlen = sizeof(socket_address);
        int conn = accept(lb_downstream_socket, (struct sockaddr*)&socket_address, (socklen_t*)&addrlen); 

        return conn;
    }


    void server_loop(){

        cout<<"Starting Load Balancer on "<<LB_IP << " at port "<< LB_PORT<<endl;
       
        // connection loop
        while(1){

            cout<<"\nWaiting for new connection from clients"<<endl;

            int downstream_connection = get_downstream_connection();

            if(downstream_connection < 0){
                cerr << "Client Connection Failed"<<endl;
                continue;
            }

            cout<<"New connection from client accepted"<<endl;

            // Scheduling
            
            int upstream_socket = schedule_upstream_socket();
            
            // start thread to handle connection traffic
            connection_threads.emplace_back(&LBScheduler::handle_connection_traffic, this, downstream_connection, upstream_socket);

            connection_threads.emplace_back(&LBScheduler::handle_connection_traffic, this, upstream_socket, downstream_connection);

    
        }

    }


    int set_nonblocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) return -1;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }




    void handle_healthcheck_requests(string server_ip, int server_port){

        ofstream healthcheck_metrics_file("healthcheck_metrics.txt." + server_ip + ":" + to_string(server_port));
        if(!healthcheck_metrics_file.is_open()){
            cerr << "Unable to open healthcheck metrics file"<<endl;
            exit(1);
        }

        while(1){
            sleep(1);
            int upstream_server_socket = create_upstream_socket(server_ip, server_port);
           
            if(upstream_server_socket < 0){
                cerr << "Connection Failed"<<endl;
                continue;
            }

            ClientRequest healthcheck_request;
            healthcheck_request.req_type = "health_check";
            healthcheck_request.req_id = "0";
            healthcheck_request.file_path = "";
            string healthcheck_request_json = healthcheck_request.convert_to_json();

            long long sent_timestamp = get_current_timestamp();
            // pad request to 1024 KB

            cout<<"Sending healthcheck request to server "<<server_ip << " at port "<< server_port <<endl;
            send(upstream_server_socket, healthcheck_request_json.c_str(), healthcheck_request_json.size(), 0);
            

            char buffer[1024] = {0};
            int read_bytes = recv(upstream_server_socket, buffer, 1024, 0);
            if(read_bytes < 0){
                cerr << "Error receiving healthcheck response"<<endl;
                continue;
            }else if(read_bytes == 0){
                cerr << "Connection closed by server"<<endl;
                continue;
            }

            //cout<<"Healthcheck response read bytes: "<<read_bytes<<endl;

            //cout<<"Healthcheck response buffer: "<<buffer<<endl;
            long long received_timestamp = get_current_timestamp();

            ServerResponse healthcheck_response;
            healthcheck_response.convert_from_json(buffer);
            cout<<"Healthcheck response received from server "<<server_ip << " at port "<< server_port <<endl;
            cout<<healthcheck_response.convert_to_json()<<endl;

            if(healthcheck_response.response_status == false){
                cerr << "Healthcheck Failed"<<endl;
                
            }

            long long response_time_ms = received_timestamp - sent_timestamp;

            healthcheck_metrics_file<< get_current_timestamp() <<","<< server_ip<<"," << server_port <<"," <<healthcheck_response.response_status<<","<< response_time_ms <<endl;

            close(upstream_server_socket);


        }

        healthcheck_metrics_file.close();

    }
 




};
