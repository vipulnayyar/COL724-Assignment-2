
#include <iostream>
#include <CLI/CLI.hpp>
#include <string>
using namespace std;

#include <assignment/lbconf.h>
#include <assignment/lbscheduler.h>

int main(int argc, char** argv){

    CLI::App app{"Load Balancer Application"};

    std::string config_file;

    app.add_option("--config-file", config_file, "(MANDATORY) Path to JSON Config file")
        ->required()
        ->check(CLI::ExistingFile);

   

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

    cout<<"\n LB_IP "<<LB_IP<<endl;
    cout<<"\n LB_PORT "<<LB_PORT<<endl;

    cout<<"\n SERVER_IPs : "<<SERVER_IP[0] <<", "<<SERVER_IP[1] <<", "<<SERVER_IP[2] <<", "<<SERVER_IP[3] <<endl;
    cout<<"\n SERVER_PORTS : "<<SERVER_PORT[0] <<", "<<SERVER_PORT[1] <<", "<<SERVER_PORT[2] <<", "<<SERVER_PORT[3] <<endl;

    cout<<endl <<"Starting LB Scheduler";
    LBScheduler load_balancer = LBScheduler(SERVER_IP[0], SERVER_PORT[0], SERVER_IP[1], SERVER_PORT[1], SERVER_IP[2], SERVER_PORT[2], SERVER_IP[3], SERVER_PORT[3], LB_IP, LB_PORT, "wrr");

    load_balancer.server_loop();
    

    cout<<endl<<"Exiting LB application";


    return 0;
}