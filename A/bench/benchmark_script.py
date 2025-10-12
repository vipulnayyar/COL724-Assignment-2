#!/usr/bin/env python3
import subprocess
import time
import csv
from pathlib import Path
import sys
from multiprocessing import Process

# ---------------- Configurable paths ----------------
BASE_DIR = Path(__file__).parent
SRC_DIR = BASE_DIR / "src"
DATA_DIR = BASE_DIR / "client_files"
RESULTS_DIR = BASE_DIR / "results"
CMDS_DIR = BASE_DIR / "cmds"
SERVER_DATA_DIR = BASE_DIR / "server_files"
SERVER_DATA_DIR.mkdir(exist_ok=True)

SERVER_EXEC = BASE_DIR / "../bin" / "server"
CLIENT_EXEC = BASE_DIR / "../bin" / "client"
CONFIG_FILE = BASE_DIR / "../conf/config.json"

# ---------------- Benchmark parameters ----------------
SCHED_POLICIES = ["fcfs", "sjf"]
SERVER_THREADS_LIST = [1, 3, 5, 7, 9]
CLIENT_THREADS_LIST = [1, 3, 5, 7, 9]
PACKET_SIZES = [3]
#NUM_RUNS = 1  # number of repetitions per configuration

WRITE_DURATION = 2  # sec
READ_DURATION = 2   # sec
TOTAL_RUN_DURATION = 8  # sec for each config

# Input files to cycle through
DATA_FILES = ["sample1.txt", "sample2.txt", "sample3.txt"]

# ---------------- Helpers ----------------
def terminate_process(proc):
    """Terminate a process gracefully"""
    proc.terminate()
    try:
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        proc.kill()
        
#file_index = 0

def run_client_for_duration(client_id, client_threads, server_threads, sched_policy, packet_size, duration_sec=TOTAL_RUN_DURATION):
    
    print("Starting client process {}", client_id)
    file_index = 0
    #global file_index
    """
    Alternates 2 sec PUT and 2 sec GET repeatedly until duration_sec
    Cycles through DATA_FILES
    Output file named based on config and run index
    """
    end_time = time.time() + duration_sec
    

    output_file = RESULTS_DIR / f"{sched_policy}_clients{client_threads}_servers{server_threads}_p{packet_size}.csv"

    while time.time() < end_time:
        
        file_name = DATA_FILES[file_index % len(DATA_FILES)]

        # PUT
        write_proc = subprocess.Popen([
            str(CLIENT_EXEC),
            "--config", str(CONFIG_FILE),
            "--mode", "single",
            "--op", "PUT",
            "--file", str(DATA_DIR / file_name),
            "--name", file_name,
            "--p", str(packet_size),
            "--client-threads", str(client_threads),
            "--server-threads", str(server_threads),
            "--sched-policy", str(sched_policy),
            "--output", str(output_file)
        ])
        write_proc.wait()

        # GET
        read_proc = subprocess.Popen([
            str(CLIENT_EXEC),
            "--config", str(CONFIG_FILE),
            "--mode", "single",
            "--op", "GET",
            "--name", file_name,
            "--p", str(packet_size),
            "--client-threads", str(client_threads),
            "--server-threads", str(server_threads),
            "--sched-policy", str(sched_policy),
            "--output", str(output_file)
        ])
        read_proc.wait()

        file_index += 1

        


def run_server(sched_policy, server_threads, packet_size):
    """Start server process with given parameters"""
    server_proc = subprocess.Popen([
        str(SERVER_EXEC),
        "--config-file", str(CONFIG_FILE),
        "--sched", sched_policy,
        "--file", str(SERVER_DATA_DIR),
        "--p", str(packet_size),
        "--server-threads", str(server_threads),
        "--quantum", str(50)
    ])
    return server_proc

# ---------------- Main Benchmark Loop ----------------
def main():
    RESULTS_DIR.mkdir(exist_ok=True)
    csv_file = RESULTS_DIR / "benchmark_results.csv"

    with open(csv_file, mode='w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            "sched_policy", "server_threads", "client_threads",
            "packet_size", "run_index", "run_time_sec"
        ])

        for sched_policy in SCHED_POLICIES:
            for server_threads in SERVER_THREADS_LIST:
                for client_threads in CLIENT_THREADS_LIST:
                    for packet_size in PACKET_SIZES:
                        #print(f"Running: {sched_policy}, S:{server_threads}, C:{client_threads}, P:{packet_size}, Run:{run_index}")

                        # Start server
                        server_proc = run_server(sched_policy, server_threads, packet_size)
                        time.sleep(2)  # give server time to start


                        client_processes = []

                        # Start clients
                        #start_time = time.time()
                        #run_client_for_duration(client_threads, server_threads, sched_policy, packet_size, run_index)
                        #end_time = time.time()


                        for i in range(client_threads):
                            p = Process(target=run_client_for_duration, args=(i, client_threads, server_threads, sched_policy, packet_size))
                            client_processes.append(p)
                        

                        for process in client_processes:
                            process.start()


                        for process in client_processes:
                            process.join()
                            




                        # Stop server
                        terminate_process(server_proc)

                        # Log results
                        """ writer.writerow([
                            sched_policy, server_threads, client_threads, packet_size,
                            run_index, round(end_time - start_time, 2)
                        ]) """




                        #print(f"Completed run: {sched_policy}, S:{server_threads}, C:{client_threads}, P:{packet_size}, Run:{run_index}")

                        #sys.exit(1)

if __name__ == "__main__":
    main()

