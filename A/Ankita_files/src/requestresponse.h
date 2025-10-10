#ifndef REQUESTRESPONSE_H
#define REQUESTRESPONSE_H

#include <string>
#include <vector>

class RequestResponse {
public:
    int req_id;
    std::string req_type;

    int arrival_time;
    int service_start_time;
    int completion_time;

    int read_line_index;
    int read_num_lines;
    std::string read_data;

    std::string file_path;
    std::string write_data;

    bool req_status;

    RequestResponse();

    // Deserialize JSON string into object fields
    void convert_from_json(const std::string& json_str);

    // Serialize object fields into JSON string
    std::string convert_to_json();
};

#endif // REQUESTRESPONSE_H

