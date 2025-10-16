# include <assignment/requestresponse.h>

class LBScheduler{
    public:

    int server1_ip = -1;
    int server1_port = -1;
    int server2_ip = -1;
    int server2_port = -1;
    int server3_ip = -1;
    int server3_port = -1;
    int server4_ip = -1;
    int server4_port = -1;

    int lb_downstream_socket = -1;


    map<int, int> lb_downstream_connections_to_upstream_sockets;


    LBScheduler(string server1_ip, int server1_port, string server2_ip, int server2_port, string server3_ip, int server3_port, string server4_ip, int server4_port, string lb_ip, int lb_port){

        lb_downstream_socket = create_downstream_socket(lb_ip, lb_port);

        server1_ip = server1_ip;
        server1_port = server1_port;
        server2_ip = server2_ip;
        server2_port = server2_port;
        server3_ip = server3_ip;
        server3_port = server3_port;
        server4_ip = server4_ip;
        server4_port = server4_port;
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
            return;
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

    int create_upstream_socket(string ip, int port){
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
            cerr << "Connection Failed"<<endl;
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
    void server_loop(string server_addr, int port){

        cout<<"Starting Load Balancer on "<<server_addr << " at port "<< port<<endl;
        int server_socket = create_listen_socket(server_addr, port);
       
        
    }



    void handle_new_downstream_connections(){

        // connection loop
        while(1){

            cout<<"\nWaiting for new connection from clients"<<endl;

            sockaddr_in socket_address;
            int addrlen = sizeof(socket_address);
            int conn = accept(lb_downstream_socket, (struct sockaddr*)&socket_address, (socklen_t*)&addrlen);

            if(conn < 0){
                cerr << "Connection Failed"<<endl;
                continue;
            }

            cout<<"New connection from client accepted"<<endl;

           
            cout<<"Request dispatched to queue";
    
  
        }

    }



    void handle_healthcheck_requests(server_ip, server_port){

        while(1){
            sleep(1);
            int upstream_server_socket = get_upstream_socket(server_ip, server_port);
           
            if(upstream_server_socket < 0){
                cerr << "Connection Failed"<<endl;
                continue;
            }

            ClientRequest healthcheck_request;
            healthcheck_request.req_type = "health_check";
            healthcheck_request.req_id = "0";
            healthcheck_request.file_path = "";
            string healthcheck_request_json = healthcheck_request.convert_to_json();

            // pad request to 1024 KB
            send(upstream_server_socket, healthcheck_request_json.c_str(), healthcheck_request_json.size(), 0);
            

            char buffer[1024] = {0};
            recv(upstream_server_socket, buffer, 1024, 0);
            ServerResponse healthcheck_response;
            healthcheck_response.convert_from_json(buffer);
            if(healthcheck_response.response_status == false){
                cerr << "Healthcheck Failed"<<endl;
                
            }

            close(upstream_server_socket);


        }

    }


    void handle_downstream_to_upstream_traffic(){



    }



    void handle_upstream_to_downstream_traffic(){

    }




};
