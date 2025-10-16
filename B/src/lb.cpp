
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

    
    if(load_lb_config(config_file) != 0){
        cerr <<"\nConfig load failure";
        return -1;
    }

    cout<<"\n LB_IP "<<LB_IP<<endl;
    cout<<"\n LB_PORT "<<LB_PORT<<endl;

    cout<<"\n SERVER1_IP"<<SERVER1_IP<<endl;
    cout<<"\n SERVER1_PORT"<<SERVER1_PORT<<endl;
    
    cout<<"\n SERVER2_IP"<<SERVER2_IP<<endl;
    cout<<"\n SERVER2_PORT"<<SERVER2_PORT<<endl;

    cout<<"\n SERVER3_IP"<<SERVER3_IP<<endl;
    cout<<"\n SERVER3_PORT"<<SERVER3_PORT<<endl;

    cout<<"\n SERVER4_IP"<<SERVER4_IP<<endl;
    cout<<"\n SERVER4_PORT"<<SERVER4_PORT<<endl;
    

    cout<<endl <<"Starting LB Scheduler";
    LBScheduler load_balancer = LBScheduler(SERVER1_IP, SERVER1_PORT, SERVER2_IP, SERVER2_PORT, SERVER3_IP, SERVER3_PORT, SERVER4_IP, SERVER4_PORT, LB_IP, LB_PORT, "rr");

    load_balancer.server_loop();
    

    cout<<endl<<"Exiting LB application";


    return 0;
}