#pragma once

#include <string>
#include <iostream>

using namespace std;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
using namespace rapidjson;



string SERVER_IP[4] = {"", "", "", ""};
int SERVER_PORT[4] = {-1, -1, -1, -1};

string LB_IP = "";
int LB_PORT = -1;

// Loading config to global vars
int load_config(string conf_file){

    Document doc;
    char buffer[8096];

    FILE *fp =fopen(conf_file.c_str(), "r");
    if(!fp){
        cerr <<"Unable to open conf file";
        return -1;
    }

    FileReadStream fs(fp, buffer, sizeof(buffer));
    doc.ParseStream(fs);

    if(doc.HasParseError()){
        cerr <<"\nNot a valid json in conf file";
        fclose(fp);
        return -1;
    }

    assert(doc.IsObject());

    assert(doc.HasMember("server1_ip"));
    assert(doc["server1_ip"].IsString());
    SERVER_IP[0] = doc["server1_ip"].GetString();

    assert(doc.HasMember("server1_port"));
    assert(doc["server1_port"].IsInt());
    SERVER_PORT[0] = doc["server1_port"].GetInt();


    assert(doc.HasMember("server2_ip"));
    assert(doc["server2_ip"].IsString());
    SERVER_IP[1] = doc["server2_ip"].GetString();

    assert(doc.HasMember("server2_port"));
    assert(doc["server2_port"].IsInt());
    SERVER_PORT[1] = doc["server2_port"].GetInt();


    assert(doc.HasMember("server3_ip"));
    assert(doc["server3_ip"].IsString());
    SERVER_IP[2] = doc["server3_ip"].GetString();

    assert(doc.HasMember("server3_port"));
    assert(doc["server3_port"].IsInt());
    SERVER_PORT[2] = doc["server3_port"].GetInt();


    assert(doc.HasMember("server4_ip"));
    assert(doc["server4_ip"].IsString());
    SERVER_IP[3] = doc["server4_ip"].GetString();

    assert(doc.HasMember("server4_port"));
    assert(doc["server4_port"].IsInt());
    SERVER_PORT[3] = doc["server4_port"].GetInt();


    
    assert(doc.HasMember("lb_ip"));
    assert(doc["lb_ip"].IsString());
    LB_IP = doc["lb_ip"].GetString();
    
    assert(doc.HasMember("lb_port"));
    assert(doc["lb_port"].IsInt());
    LB_PORT = doc["lb_port"].GetInt();


    return 0;
}
