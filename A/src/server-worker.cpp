
#include <fstream>
#include "requestresponse.cpp"
class ServerWorker{

    public:

    int connection;
    RequestResponse request;


    ServerWorker(){


    }

    vector<RequestResponse> process_req( RequestResponse req){

       // connection = conn;
        request = req;

        if(request.req_type == "read"){
            return read();
        }
        else if(request.req_type=="write"){

            return write();
        }

    }

    vector<RequestResponse> read(){

        
    }

    vector<RequestResponse> write(){

        ofstream outputFile(request.file_path);

        if (outputFile.is_open()) {
            outputFile << request.write_data;

            outputFile << endl;

            outputFile.close();
            cout << "Data written to " << request.file_path << endl;
        } else {
            cerr << "Unable to open file " << request.file_path << endl;
        }



        request.write_data = "";
        request.req_status = true;

        vector<RequestResponse> responses;
        responses.push_back(request);

        return responses;




    }
};