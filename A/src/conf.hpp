
#include <string>

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

    assert(doc.HasMember("server_ip"));
    assert(doc["server_ip"].IsString());

    assert(doc.HasMember("server_port"));
    assert(doc["server_port"].IsInt());

    assert(doc.HasMember("server_threads"));
    assert(doc["server_threads"].IsInt());
    
    assert(doc.HasMember("client_threads"));
    assert(doc["client_threads"].IsInt());

    return 0;
}
