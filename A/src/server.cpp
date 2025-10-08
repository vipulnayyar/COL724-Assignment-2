
#include <iostream>

#include "CLI/CLI.hpp"

#include <string>
using namespace std;
#include "conf.hpp"


class Worker{


    
};

class Scheduler{


};

class Server{
    public:

        Server(string sched_policy){

        }

        void server_loop(){

            while(1){
                ;
            }
        }
};


int main(int argc, char** argv){

    CLI::App app{"Server Application"};

    std::string config_file;

    app.add_option("--config-file", config_file, "(MANDATORY) Path to JSON Config file")
        ->required()
        ->check(CLI::ExistingFile);

    std::string sched_policy;

    app.add_option("--sched", sched_policy, "(MANDATORY) Scheduling policy for server")
        ->check(CLI::IsMember({"fcfs", "sjf", "rr"}))
        ->required();

    std::string file_store_dir;
    app.add_option("--file", file_store_dir, "(MANDATORY) Directory path for file data store")
        ->check(CLI::ExistingDirectory)
        ->required();


    int packet_size;
    app.add_option("--p", packet_size, "(MANDATORY) Lines of text for server to send in one packet")
        ->required()
        ->check(CLI::Range(1, std::numeric_limits<int>::max()));

    int rr_quantum;
    app.add_option("--quantum", rr_quantum, "(MANDATORY for RR) Time quantum for Round-Robin scheduling ")
        ->check(CLI::Range(1, std::numeric_limits<int>::max()));


    int cmd_server_threads;
    app.add_option("--server-threads", cmd_server_threads, "(OPTIONAL) Number of worker threads the server creates(overrides value from config file)")
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


    if (sched_policy == "rr" && app.get_option("--quantum")->count() == 0) {
        cerr << "Error: --quantum is required when --sched is 'rr'\n\n";
        cerr << app.help() <<endl;   
        return 1;
    }

    
    if(load_config(config_file) != 0){
        cerr <<"\nConfig load failure";
        return -1;
    }

    if (app.get_option("--server-threads")->count() != 0 ){
        SERVER_THREADS = cmd_server_threads;
    }

    if (app.get_option("--client-threads")->count() != 0 ){
        SERVER_THREADS = cmd_client_threads;
    }


    return 0;
}