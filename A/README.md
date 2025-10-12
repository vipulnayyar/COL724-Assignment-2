# COL724-Assignment-2

## Build
```
$ make
```
The server and client executables are created in `./bin` dir


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
          --quantum INT:INT in [1 - 2147483647]
                              (MANDATORY for RR) Time quantum for Round-Robin scheduling
          --server-threads INT:INT in [1 - 2147483647]
                              (OPTIONAL) Number of worker threads the server creates(overrides
                              value from config file)


$ ./bin/server --config-file ./conf/config.json --sched fcfs --file ./bench/server_files --p 3 --server-threads 1
```

## Start Client

```
$ ./bin/client --help
Usage examples:
./bin/client --config config.json --mode single --op PUT --file ./data/test.txt --name test.txt --p 5
./bin/client --config config.json --mode single --op GET --name test.txt --p 5
./bin/client --config config.json --mode batch --commands cmds.txt --client-threads 8 --iterations 10 --p 5 --output results.csv

```


## Run Benchmarks

```
$ cd bench/results
$ rm *.csv
$ cd ..
$ python3 benchmark_script.py
$ cat results/*.csv > combined.csv
```
The benchmark files are individually written for each unique config in `bench/results` dir. They are combined in one csv and used for graph generation.