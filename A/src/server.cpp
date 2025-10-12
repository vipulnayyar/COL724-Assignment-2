
#include <iostream>

#include <CLI/CLI.hpp>

#include <string>
using namespace std;
#include <assignment/conf.h>


#include <assignment/scheduler.h>


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

    int rr_quantum = 0;
    app.add_option("--quantum", rr_quantum, "(MANDATORY for RR) Time quantum for Round-Robin scheduling ")
        ->check(CLI::Range(1, std::numeric_limits<int>::max()));


    int cmd_server_threads;
    app.add_option("--server-threads", cmd_server_threads, "(OPTIONAL) Number of worker threads the server creates(overrides value from config file)")
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


    if (sched_policy != "rr")
        rr_quantum = 0;

    cout<<endl<<"SERVER CONFIG : "<<endl;
    cout<<"SERVER_IP : "<< SERVER_IP<<endl;
    cout<<"SERVER_PORT : "<<SERVER_PORT<<endl;
    cout<<"SERVER_THREADS : "<<SERVER_THREADS<<endl;
    cout<<"CLIENT_THREADS : "<<CLIENT_THREADS<<endl;

    cout<<"config_file : "<<config_file<<endl;
    cout<<"sched_policy : "<<sched_policy<<endl;
    cout<<"file_store_dir : "<<file_store_dir<<endl;
    cout<<"packet_size : "<<packet_size<<endl;
    cout<<"rr_quantum : "<<rr_quantum<<endl;
   
    
    if(sched_policy == "sjf"){

        cout<<endl <<"Starting SJF Scheduler";
        SJFScheduler scheduler = SJFScheduler(SERVER_THREADS, file_store_dir, packet_size, 0);

        scheduler.server_loop(SERVER_IP, SERVER_PORT);

    }else if(sched_policy == "rr"){

        cout<<endl <<"Starting RR Scheduler";

        RRScheduler scheduler = RRScheduler(SERVER_THREADS, file_store_dir, packet_size, rr_quantum);

        scheduler.server_loop(SERVER_IP, SERVER_PORT);

    }else{

        cout <<"Starting FCFS Scheduler"<<endl;

        FCFSScheduler scheduler = FCFSScheduler(SERVER_THREADS, file_store_dir, packet_size, 0);

        scheduler.server_loop(SERVER_IP, SERVER_PORT);
    }
    




    

    cout<<endl<<"Exiting Server application";


    return 0;
}