#pragma once

#include <string>
#include <iostream>

using namespace std;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
using namespace rapidjson;

#include <assignment/conf.h>

string SERVER1_IP = "";
int SERVER1_PORT = -1;

string SERVER2_IP = "";
int SERVER2_PORT = -1

string SERVER3_IP = "";
int SERVER3_PORT = -1

string SERVER4_IP = "";
int SERVER4_PORT = -1

string LB_IP = "";
int LB_PORT = -1

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
    SERVER1_IP = doc["server1_ip"].GetString();

    assert(doc.HasMember("server1_port"));
    assert(doc["server1_port"].IsInt());
    SERVER1_PORT = doc["server1_port"].GetInt();


    assert(doc.HasMember("server2_ip"));
    assert(doc["server2_ip"].IsString());
    SERVER2_IP = doc["server2_ip"].GetString();

    assert(doc.HasMember("server2_port"));
    assert(doc["server2_port"].IsInt());
    SERVER2_PORT = doc["server2_port"].GetInt();


    assert(doc.HasMember("server3_ip"));
    assert(doc["server3_ip"].IsString());
    SERVER3_IP = doc["server3_ip"].GetString();

    assert(doc.HasMember("server3_port"));
    assert(doc["server3_port"].IsInt());
    SERVER3_PORT = doc["server3_port"].GetInt();


    assert(doc.HasMember("server4_ip"));
    assert(doc["server4_ip"].IsString());
    SERVER4_IP = doc["server4_ip"].GetString();

    assert(doc.HasMember("server4_port"));
    assert(doc["server4_port"].IsInt());
    SERVER4_PORT = doc["server4_port"].GetInt();


    
    assert(doc.HasMember("lb_ip"));
    assert(doc["lb_ip"].IsInt());
    LB_IP = doc["lb_ip"].GetInt();
    
    assert(doc.HasMember("lb_port"));
    assert(doc["lb_port"].IsInt());
    LB_PORT = doc["lb_port"].GetInt();


    return 0;
}
