# COL724-Assignment-2 Part B

## Build
```
$ make
```
The Server, Client and Load Balancer executables are created in `./bin` dir.

## Start Server
```
$ ./bin/server --help
Server Application


./bin/server [OPTIONS]


OPTIONS:
  -h,     --help              Print this help message and exit
          --config-file TEXT:FILE REQUIRED
                              (MANDATORY) Path to JSON Config file
          --sched TEXT:{fcfs,sjf,rr} REQUIRED
                              (MANDATORY) Scheduling policy for server
          --file TEXT:DIR REQUIRED
                              (MANDATORY) Directory path for file data store
          --p INT:INT in [1 - 2147483647] REQUIRED
                              (MANDATORY) Lines of text for server to send in one packet
          --server-id INT:INT in [1 - 4] REQUIRED
                              (MANDATORY) Server ID(1-4)
          --quantum INT:INT in [1 - 2147483647]
                              (MANDATORY for RR) Time quantum for Round-Robin scheduling
          --server-threads INT:INT in [1 - 2147483647]
                              (OPTIONAL) Number of worker threads the server creates(overrides
                              value from config file)


$ ./bin/server --config-file ./conf/config.json --sched fcfs --file ./bench/server_files --p 2 --server-id 1

```

## Start Client

```
$ ./bin/client --help
Usage: ./bin/client [OPTIONS]
Options:
  --op <GET|PUT>         Operation type (required)
  --file <path>          Local file path for PUT operation
  --name <name>          Server-side file name
  --config <path>        Path to config.json (default: ./conf/config.json)
  --req-id <id>          Request ID (default: 1)
Example:
  ./bin/client --req-id <req_id> --op GET --name sample2.txt --config ./conf/config.json

```

## Run Benchmarks

```
$ cd bench/

# Run benchmarks for Weighted Round Robin LB scheduling
$ ./bench.sh wrr

# Run benchmarks for Least Connections LB scheduling
$ ./bench.sh lc
```
Load Balancer execution logs are present in `./bench/lb/`
Server1 execution logs are present in `./bench/server1/`
Server2 execution logs are present in `./bench/server2/`
Server3 execution logs are present in `./bench/server3/`
Server4 execution logs are present in `./bench/server4/`
Client execution logs are present in `./bench/client/`



