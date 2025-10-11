#ifndef FILE_IO_H
#define FILE_IO_H

#include <string>
using namespace std;

// Just declare functions here — no code.
string read_file(string filepath, int start_line, int num_lines);
void write_file(string filepath, string data);

#endif

