

string read_file(string filepath, int read_line_index, int num_lines_to_read){

    ifstream infile(filepath);

    if(!infile.is_open()){
        cerr << "Unable to open file for reading"<<filepath<<endl;
    }


    cout<<"Reading file : "<<filepath<<endl;

    string line;
    int current_line = 0;

    while (current_line < read_line_index - 1 && getline(infile, line)) {
        current_line++;
    }

    string result = "";
    int lines_read = 0;


    while (lines_read < num_lines_to_read && getline(infile, line)) {
        result += line;
    
        if (lines_read < num_lines_to_read - 1) { 
            result += "\n";
        }
    
        lines_read++;

    }


    infile.close();

    cout<<"Result of file reading : "<<result<<endl;

    return result;

}


void write_file(string filepath, string write_data){

    ofstream outfile(filepath);

    if (outfile.is_open()) {
        outfile << write_data;

        outfile.close();

        cout << "Data written to " << filepath << endl;
    } else {
        cerr << "Unable to open file for writing" << filepath << endl;
    }


}