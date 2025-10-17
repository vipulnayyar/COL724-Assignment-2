#pragma once

#include <string>
#include <iostream>

using namespace std;


#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
using namespace rapidjson;


string SERVER_IP = "";
int SERVER_PORT = -1;
int SERVER_THREADS = -1;
int CLIENT_THREADS = -1;

// Loading config to global vars
int load_server_config(string conf_file){

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

    assert(doc.HasMember("server_ip"));
    assert(doc["server_ip"].IsString());
    SERVER_IP = doc["server_ip"].GetString();

    assert(doc.HasMember("server_port"));
    assert(doc["server_port"].IsInt());
    SERVER_PORT = doc["server_port"].GetInt();

    assert(doc.HasMember("server_threads"));
    assert(doc["server_threads"].IsInt());
    SERVER_THREADS = doc["server_threads"].GetInt();
    
    assert(doc.HasMember("client_threads"));
    assert(doc["client_threads"].IsInt());
    CLIENT_THREADS = doc["client_threads"].GetInt();

    return 0;
}
