#!/usr/bin/env python3
import subprocess, time, os, signal, random, math, threading, json
from datetime import datetime


# Experiment Parameters

ALGORITHMS     = ["wrr", "wlc"]           # weighted round-robin and weighted-least-connection
CLIENT_THREADS = [2, 4, 6]             # concurrency levels
REQUEST_RATES  = [10, 50, 100, 200]      # mean Poisson rates (req/s per client)
RUN_TIME       = 60                      # seconds per experiment
CONFIG_PATH    = "./conf/config.json"           # must contain lb + 4 servers
DATA_FILE      = "./data/sample.txt"     # test file for PUT/GET
LOG_DIR        = "./logs"


# Utility helpers
def ensure_dirs():
    os.makedirs(LOG_DIR, exist_ok=True)
    os.makedirs(f"{LOG_DIR}/servers", exist_ok=True)
    os.makedirs(f"{LOG_DIR}/clients", exist_ok=True)
    os.makedirs(f"{LOG_DIR}/lb", exist_ok=True)

def read_config():
    with open(CONFIG_PATH) as f:
        return json.load(f)

def spawn(cmd, logfile):
    """Launch process and redirect output to logfile."""
    with open(logfile, "w") as out:
        proc = subprocess.Popen(cmd, stdout=out, stderr=subprocess.STDOUT)
    return proc

def kill(proc):
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
    except Exception:
        pass

# Start / Stop Component
def start_servers(cfg):
    procs = []
    for i in range(1, 5):
        ip   = cfg[f"server{i}_ip"]
        port = str(cfg[f"server{i}_port"])
        logfile = f"{LOG_DIR}/servers/server{i}.log"
        cmd = ["./server", "--ip", ip, "--port", port,
               "--sched", "fcfs", "--file", DATA_FILE, "--p", "5"]
        procs.append(spawn(cmd, logfile))
        time.sleep(0.5)
    print("[+] Servers started.")
    return procs

def start_loadbalancer(cfg, algo):
    logfile = f"{LOG_DIR}/lb/lb_{algo}.log"
    cmd = ["./loadbalancer", "--config", CONFIG_PATH, "--algo", algo]
    proc = spawn(cmd, logfile)
    print(f"[+] Load balancer started with algorithm: {algo}")
    time.sleep(3)  # allow health-check cycle
    return proc

def start_clients(cfg, n_clients, rate):
    lb_ip   = cfg["lb_ip"]
    lb_port = str(cfg["lb_port"])
    threads = []
    stop_flag = threading.Event()

    def client_thread(cid):
        logfile = f"{LOG_DIR}/clients/client_{cid}.log"
        with open(logfile, "w") as out:
            start_time = time.time()
            while not stop_flag.is_set() and (time.time() - start_time < RUN_TIME):
                inter = -math.log(1.0 - random.random()) / rate  # exponential(λ)
                time.sleep(inter)
                cmd = ["./client", "--config", CONFIG_PATH,
                       "--file", DATA_FILE, "--name", f"sample_{cid}.txt", "--op", "PUT"]
                subprocess.run(cmd, stdout=out, stderr=subprocess.STDOUT)

    for cid in range(n_clients):
        t = threading.Thread(target=client_thread, args=(cid,))
        t.start()
        threads.append(t)

    return threads, stop_flag

def stop_all(lbp, server_procs, stop_flag, client_threads):
    stop_flag.set()
    for t in client_threads: t.join()
    kill(lbp)
    for p in server_procs: kill(p)
    print("All processes terminated.\n")
    
    
# Main Benchmark Loop

def run_benchmarks():
    ensure_dirs()
    cfg = read_config()

    for algo in ALGORITHMS:
        for n_clients in CLIENT_THREADS:
            for rate in REQUEST_RATES:
                print(f"\n=== Running {algo.upper()} | Clients={n_clients} | Rate={rate} req/s ===")
                servers = start_servers(cfg)
                lb_proc = start_loadbalancer(cfg, algo)
                clients, stop_flag = start_clients(cfg, n_clients, rate)

                # let run_time elapse
                time.sleep(RUN_TIME)

                stop_all(lb_proc, servers, stop_flag, clients)
                time.sleep(3)  # cool-down between runs

    print("\n Benchmarking completed. Logs in:", LOG_DIR)

# ─────────────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    run_benchmarks()

