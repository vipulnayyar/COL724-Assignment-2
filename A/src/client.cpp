
#include <iostream>

#include "CLI/CLI.hpp"

#include <string>
using namespace std;
#include "conf.cpp"

#include "requestresponse.cpp"
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

class Worker{
    public:
    

};



int main(int argc, char** argv){

    CLI::App app{"Client Application"};

    std::string config_file;

    app.add_option("--config-file", config_file, "(MANDATORY) Path to JSON Config file")
        ->required()
        ->check(CLI::ExistingFile);




    int packet_size;
    app.add_option("--p", packet_size, "(MANDATORY) Lines of text for server to send in one packet")
        ->required()
        ->check(CLI::Range(1, std::numeric_limits<int>::max()));


    int cmd_client_threads;
    app.add_option("--client-threads", cmd_client_threads, "(OPTIONAL) Number of concurrent client instances(overrides value from config file)")
        ->check(CLI::Range(1, std::numeric_limits<int>::max()));


    try{
        CLI11_PARSE(app, argc, argv);
    }
    catch (const CLI::Error &err) {
        return app.exit(err);
    }

    
    if(load_config(config_file) != 0){
        cerr <<"\nConfig load failure";
        return -1;
    }

    if (app.get_option("--client-threads")->count() != 0 ){
        CLIENT_THREADS = cmd_client_threads;
    }


    cout<<endl<<"Client CONFIG : "<<endl;
    cout<<"SERVER_IP : "<< SERVER_IP<<endl;
    cout<<"SERVER_PORT : "<<SERVER_PORT<<endl;
    cout<<"SERVER_THREADS : "<<SERVER_THREADS<<endl;
    cout<<"CLIENT_THREADS : "<<CLIENT_THREADS<<endl;

    cout<<"config_file : "<<config_file<<endl;
    cout<<"packet_size : "<<packet_size<<endl;

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(client_socket <= 0){
        cerr << "Socket Creation Failed";
        exit(1);
    }

    sockaddr_in socket_address;
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = inet_addr(SERVER_IP.c_str());
    socket_address.sin_port = htons(SERVER_PORT);


    string mode = "sending";

    while(1){
    
            connect(client_socket, (struct sockaddr*)&socket_address,
                    sizeof(socket_address));

            
            RequestResponse req;
            req.req_id = 1;
            req.req_type = "write";
            req.file_path ="/tmp/test";
            req.write_data = "test12\n3";

            string json_msg = req.convert_to_json();

            cout<< endl<<"Sending"<<json_msg<<endl;

            send(client_socket, json_msg.c_str(), json_msg.size(), 0);
            
      

            char msg_buffer[1000] = {0};
            auto bytesRead = read(client_socket, msg_buffer, 1000);
            cout << "The message received from server : " << msg_buffer<<endl;

         


    }
    //close(client_socket);


    return 0;
}