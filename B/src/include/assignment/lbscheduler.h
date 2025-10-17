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
#include <atomic>
#include <mutex>

using namespace std;
using namespace std::chrono;



class LBScheduler{
    public:

    string server_ip[4] = {"", "", "", ""};
    int server_port[4] = {-1, -1, -1, -1};


    string lb_ip = "";
    int lb_port = -1;

    int lb_downstream_socket = -1;

    string sched_policy = "wrr";

    vector<thread> connection_threads;

    vector<thread> healthcheck_threads;

    // Adaptive Weighted Round Robin (1-10)
    int server_weight[4] = {1, 1, 1, 1};

    // Adaptive Least Connections
    atomic<int> server_connections[4] = {0, 0, 0, 0};

    mutex mutex_average_time_spent_on_server;

    float average_time_spent_on_server[4] = {0, 0, 0, 0};


    int get_bucket_index(float value, float min_value, float max_value){

        cout<<"Value: "<<value<<", Min Value: "<<min_value<<", Max Value: "<<max_value<<endl;

        int BUCKET_COUNT = 10;
        float diff = max_value - min_value;
        float diff_bucket = diff / BUCKET_COUNT;

        if(diff_bucket == 0)
            return 1;

        int bucket_index = ((value - min_value) / diff_bucket) + 1;

        if(bucket_index > BUCKET_COUNT){
            bucket_index = BUCKET_COUNT;
        }

        return (BUCKET_COUNT + 1 - bucket_index);
    }


    void rebalance_wrr_weights(){


        mutex_average_time_spent_on_server.lock();
        float min_average_time_spent_on_server = average_time_spent_on_server[0];
        int min_average_time_spent_on_server_index = 0;

        for(int i = 1; i < 4; i++){
            if(average_time_spent_on_server[i] < min_average_time_spent_on_server){
                min_average_time_spent_on_server = average_time_spent_on_server[i];
                min_average_time_spent_on_server_index = i;
            }
        }

        float max_average_time_spent_on_server = average_time_spent_on_server[0];
        int max_average_time_spent_on_server_index = 0;

        for(int i = 1; i < 4; i++){
            if(average_time_spent_on_server[i] > max_average_time_spent_on_server){
                max_average_time_spent_on_server = average_time_spent_on_server[i];
                max_average_time_spent_on_server_index = i;
            }
        }


        
        for(int i = 0; i < 4; i++){
            int bucket_index = get_bucket_index(average_time_spent_on_server[i], min_average_time_spent_on_server, max_average_time_spent_on_server);

            server_weight[i] = 0.9* server_weight[i] + 0.1* bucket_index;


            cout<<"Server "<<i+1<<" weight: "<<server_weight[i]<<endl;
        }

        mutex_average_time_spent_on_server.unlock();
        



       
        
    }



    void rebalance_backends(){
        if(sched_policy == "wrr"){
            rebalance_wrr_weights();
        }else if(sched_policy == "lc"){
            //rebalance_lc_weights();
        }
    }


    long long get_current_timestamp(){
        auto timestamp_now = system_clock::now();               
        auto timestamp_ms = duration_cast<milliseconds>(         
            timestamp_now.time_since_epoch()
        ).count();

        return timestamp_ms;

    }

    LBScheduler(string srv1_ip, int srv1_port, string srv2_ip, int srv2_port, string srv3_ip, int srv3_port, string srv4_ip, int srv4_port, string load_balancer_ip, int load_balancer_port, string lb_sched_policy){

        lb_downstream_socket = create_downstream_socket(load_balancer_ip, load_balancer_port);

        if(lb_downstream_socket < 0){
            cerr << "***Downstream socket creation failed***"<<endl;
            exit(1);
        }

        cout<<"Downstream socket created successfully"<<endl;

        server_ip[0] = srv1_ip;
        server_port[0] = srv1_port;
        server_ip[1] = srv2_ip;
        server_port[1] = srv2_port;
        server_ip[2] = srv3_ip;
        server_port[2] = srv3_port;
        server_ip[3] = srv4_ip;
        server_port[3] = srv4_port;

        lb_ip = load_balancer_ip;
        lb_port = load_balancer_port;

        sched_policy = lb_sched_policy;


        for(int i = 0; i < 4; i++){
            healthcheck_threads.emplace_back(&LBScheduler::handle_healthcheck_requests, this, i);
        }

    }



    void handle_connection_traffic( int upstream_server_id, string direction, int source_connection, int destination_connection){

        if(direction == "upward"){
            server_connections[upstream_server_id].fetch_add(1, memory_order_relaxed);
        }

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

            ClientRequest client_request;
            client_request.convert_from_json(buffer);

            cout<<"\n Request received from client: "<<client_request.req_id<<endl;

            int send_bytes = send(destination_connection, buffer, read_bytes, 0);
            if(send_bytes < 0){
                cerr << "Error sending to destination connection"<<endl;
                break;
            }else if(send_bytes == 0){
                cerr << "Connection closed by destination connection"<<endl;
                break;
            }
            else{
                //cout<<"Request sent to server"<<endl;
                cout<<"Request sent to upstream server: "<<client_request.req_id<<endl;
            }
        }


        close(source_connection);
        close(destination_connection);

        if(direction == "upward"){
            server_connections[upstream_server_id].fetch_sub(1, memory_order_relaxed);
        }

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
        if(bind_status < 0){
            cerr << "***Bind Failed for downstream socket***"<<endl;
            return -1;
        }

        cout<<"Bind status = "<<bind_status <<endl;

        int listen_status = listen(downstream_socket, 64);
        if(listen_status < 0){
            cerr << "***Listen Failed for downstream socket***"<<endl;
            return -1;
        }

        cout<<"Listen status = "<<listen_status <<endl;

        return downstream_socket;
    }

    int schedule_weighted_round_robin_server(){
        static int round_robin_index = 0;
        round_robin_index++;

        cout<<" Current server weights: "<<server_weight[0]<<", "<<server_weight[1]<<", "<<server_weight[2]<<", "<<server_weight[3]<<endl;

        int current_server_index = round_robin_index % (server_weight[0] + server_weight[1] + server_weight[2] + server_weight[3]);

        if(current_server_index < server_weight[0]){
            return 0;
            //return {server1_ip, server1_port};
        }else if(current_server_index < server_weight[0] + server_weight[1]){
            return 1;
            //return {server2_ip, server2_port};
        }else if(current_server_index < server_weight[0] + server_weight[1] + server_weight[2]){
            return 2;
            //return {server3_ip, server3_port};
        }else{
            return 3;
            //return {server4_ip, server4_port};
        }   

    }


    int schedule_weighted_least_connections_server(){

        int min_connections = server_connections[0];
        int min_connections_index = 0;

        for(int i = 1; i < 4; i++){
            if(server_connections[i] < min_connections){
                min_connections = server_connections[i];
                min_connections_index = i;
            }
        }

        int max_connections = server_connections[0];
        int max_connections_index = 0;

        for(int i = 1; i < 4; i++){
            if(server_connections[i] > max_connections){
                max_connections = server_connections[i];
                max_connections_index = i;
            }
        }

        

        return min_connections_index;

        /* if (min_connections_index == 0){
            return {server1_ip, server1_port};
        }else if(min_connections_index == 1){
            return {server2_ip, server2_port};
        }else if(min_connections_index == 2){
            return {server3_ip, server3_port};
        }else{
            return {server4_ip, server4_port};
        } */
    }

    int schedule_upstream_server_id(){
        if(sched_policy == "wrr"){
            return  schedule_weighted_round_robin_server();

            //return create_upstream_socket(server_info.first, server_info.second);

        }else if(sched_policy == "lc"){

            return  schedule_weighted_least_connections_server();

            //return create_upstream_socket(server_info.first, server_info.second);

        }else{
            return 0;
        }
    }

    int create_upstream_socket(string ip, int port){


        //cout<<"\nCreating upstream socket to server "<<ip << " at port "<< port <<endl;

        int upstream_socket = socket(AF_INET, SOCK_STREAM, 0);

        if(upstream_socket <= 0){
            cerr << "Socket Creation Failed"<<endl;
            return -1;
        }

        sockaddr_in socket_address;
        socket_address.sin_family = AF_INET;
        socket_address.sin_addr.s_addr = inet_addr(ip.c_str());
        socket_address.sin_port = htons(port);

        int connect_status = connect(upstream_socket, (struct sockaddr*)&socket_address, sizeof(sockaddr));
        if(connect_status < 0){
            cerr << "Upstream Connection Failed"<<endl;
            return -1;
        }

        //cout<<"Upstream Connection from LB to Server "<<ip << " at port "<< port <<" Successful"<<endl;

        return upstream_socket;
    }

    int get_downstream_connection(){
        
        sockaddr_in socket_address;
        socket_address.sin_family = AF_INET;
        socket_address.sin_addr.s_addr = inet_addr(lb_ip.c_str());
        socket_address.sin_port = htons(lb_port);

        int addrlen = sizeof(socket_address);
        int conn = accept(lb_downstream_socket, (struct sockaddr*)&socket_address, (socklen_t*)&addrlen); 

        return conn;
    }


    void server_loop(){

        cout<<"Starting Load Balancer on "<<lb_ip << " at port "<< lb_port<<endl;
       
        // connection loop
        while(1){

            cout<<"\nWaiting for new connection from clients"<<endl;

            int downstream_connection = get_downstream_connection();

            if(downstream_connection < 0){
                cerr << "Client Connection Failed"<<endl;
                continue;
            }

            cout<<"New connection from client accepted"<<endl;

            
            


            int upstream_socket = -1;
            int upstream_server_id = -1;

            int count = 0;

            while(1){

                count++;
                upstream_server_id = schedule_upstream_server_id();
                upstream_socket = create_upstream_socket(server_ip[upstream_server_id], server_port[upstream_server_id]);
                if(upstream_socket < 0){
                    cerr << "Connection to upstream server ID "<<upstream_server_id+1<<" Failed"<<endl;
                    cout<<"Trying to connect to next upstream server"<<endl;

                    if(count >= 4){
                        cerr << "All upstream servers are down"<<endl;
                        exit(1);
                    }

                }else{
                    cout<<"Connection to upstream server ID "<<upstream_server_id+1<<" Successful"<<endl;
                    break;
                }

                


            }

            
            connection_threads.emplace_back(&LBScheduler::handle_connection_traffic, this, upstream_server_id, "upward", downstream_connection, upstream_socket);
            connection_threads.emplace_back(&LBScheduler::handle_connection_traffic, this, upstream_server_id, "downward", upstream_socket, downstream_connection);

            // Rebalancing WRR weights
            rebalance_wrr_weights();



        }
    }

    int set_nonblocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) return -1;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }




    void handle_healthcheck_requests(int upstream_server_id){

        ofstream healthcheck_metrics_file("healthcheck_metrics.txt." + server_ip[upstream_server_id] + ":" + to_string(server_port[upstream_server_id]));
        if(!healthcheck_metrics_file.is_open()){
            cerr << "Unable to open healthcheck metrics file"<<endl;
            exit(1);
        }

        while(1){
            sleep(1);
            int upstream_server_socket = create_upstream_socket(server_ip[upstream_server_id], server_port[upstream_server_id]);
           
            if(upstream_server_socket < 0){
                cerr << "Connection Failed"<<endl;

                mutex_average_time_spent_on_server.lock();
                average_time_spent_on_server[upstream_server_id] = 999999.0;
                mutex_average_time_spent_on_server.unlock();


                healthcheck_metrics_file<< get_current_timestamp() <<","<< upstream_server_id + 1 <<"," << server_ip[upstream_server_id]<<"," << server_port[upstream_server_id] <<"," <<false<<","<< 0 <<","<< 0 <<","<< 0 << "," << 0 << endl;

                continue;
            }

            ClientRequest healthcheck_request;
            healthcheck_request.req_type = "health_check";
            healthcheck_request.req_id = "0";
            healthcheck_request.file_path = "";
            string healthcheck_request_json = healthcheck_request.convert_to_json();

            long long sent_timestamp = get_current_timestamp();
            // pad request to 1024 KB

            //cout<<"Sending healthcheck request to server "<<server_ip << " at port "<< server_port <<endl;
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

            HealthCheckResponse healthcheck_response;
            healthcheck_response.convert_from_json(buffer);
            //cout<<"Healthcheck response received from server "<<server_ip << " at port "<< server_port <<endl;
            //cout<<healthcheck_response.convert_to_json()<<endl;


            mutex_average_time_spent_on_server.lock();
            average_time_spent_on_server[upstream_server_id] = healthcheck_response.average_time_spent_on_server;
            mutex_average_time_spent_on_server.unlock();

            if(healthcheck_response.response_status == false){
                cerr << "Healthcheck Failed"<<endl;
                
            }

            long long response_time_ms = received_timestamp - sent_timestamp;

            healthcheck_metrics_file<< get_current_timestamp() <<","<< upstream_server_id + 1 <<"," << server_ip[upstream_server_id]<<"," << server_port[upstream_server_id] <<"," <<healthcheck_response.response_status<<","<< response_time_ms <<","<< healthcheck_response.queue_length <<","<< healthcheck_response.connection_throughput << "," << healthcheck_response.average_time_spent_on_server << endl;


            close(upstream_server_socket);


        }

        healthcheck_metrics_file.close();

    }
 




};
