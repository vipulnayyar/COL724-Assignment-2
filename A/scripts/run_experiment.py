#!/usr/bin/env python3
# (Large simulation script — creates per-request CSVs for fcfs, sjf, rr)
# Paste the exact content below (it's the simulation runner).
# [BEGIN SCRIPT]
import argparse, json, os, csv, math, random, uuid
from collections import deque
import heapq
from datetime import datetime
import pandas as pd

def read_config(cfgfile="config.json"):
    try:
        with open(cfgfile) as f:
            return json.load(f)
    except Exception:
        return {}

class Request:
    def __init__(self, request_id, client_id, arrival, service_time):
        self.request_id = request_id
        self.client_id = client_id
        self.arrival = arrival
        self.service = service_time
        self.remaining = service_time
        self.start = None
        self.finish = None
        self.first_start_recorded = False

def gen_requests(n_clients, reqs_per_client=20, interarrival_mean=0.05, base_service_mean=0.1, p=5):
    requests = []
    rid = 0
    for c in range(n_clients):
        t = 0.0
        for i in range(reqs_per_client):
            t += random.expovariate(1.0 / interarrival_mean)
            service = random.expovariate(1.0 / (base_service_mean * max(1, p)))
            requests.append(Request(f"r{rid}", f"c{c}", round(t,6), round(service,6)))
            rid += 1
    requests.sort(key=lambda r: r.arrival)
    return requests

def simulate_fcfs(requests, n_workers):
    workers = [0.0] * n_workers
    records = []
    for req in requests:
        idx = min(range(n_workers), key=lambda i: workers[i])
        start = max(req.arrival, workers[idx])
        req.start = start
        req.finish = start + req.service
        workers[idx] = req.finish
        records.append(req)
    return records

def simulate_sjf(requests, n_workers):
    workers = [0.0] * n_workers
    pending = []
    records = []
    arrivals = list(requests)
    ai = 0
    n = len(arrivals)
    while ai < n or pending:
        next_arrival = arrivals[ai].arrival if ai < n else math.inf
        next_worker_free = min(workers) if pending else math.inf
        if next_arrival <= next_worker_free:
            r = arrivals[ai]; ai += 1
            heapq.heappush(pending, (r.service, id(r), r))
        else:
            idx = min(range(n_workers), key=lambda i: workers[i])
            free_time = workers[idx]
            if pending:
                _, _, job = heapq.heappop(pending)
                start = max(job.arrival, free_time)
                job.start = start
                job.finish = start + job.service
                workers[idx] = job.finish
                records.append(job)
            else:
                pass
    all_processed = set(r.request_id for r in records)
    for r in requests:
        if r.request_id not in all_processed:
            idx = min(range(n_workers), key=lambda i: workers[i])
            start = max(r.arrival, workers[idx])
            r.start = start
            r.finish = start + r.service
            workers[idx] = r.finish
            records.append(r)
    records.sort(key=lambda x: x.arrival)
    return records

def simulate_rr(requests, n_workers, quantum=0.2):
    workers = [0.0] * n_workers
    pending = deque()
    records = []
    arrivals = list(requests)
    ai = 0
    while ai < len(arrivals) or pending:
        if not pending and ai < len(arrivals):
            time = arrivals[ai].arrival
            pending.append(arrivals[ai]); ai+=1
            while ai < len(arrivals) and arrivals[ai].arrival <= time:
                pending.append(arrivals[ai]); ai+=1
        idx = min(range(n_workers), key=lambda i: workers[i])
        start_time = max(workers[idx], pending[0].arrival if pending else 0.0)
        if ai < len(arrivals) and arrivals[ai].arrival <= start_time and arrivals[ai].arrival <= workers[idx]:
            while ai < len(arrivals) and arrivals[ai].arrival <= start_time:
                pending.append(arrivals[ai]); ai+=1
        if not pending:
            if ai < len(arrivals):
                pending.append(arrivals[ai]); ai+=1
            else:
                break
        job = pending.popleft()
        slice_t = min(job.remaining, quantum)
        if not job.first_start_recorded:
            job.start = start_time
            job.first_start_recorded = True
        exec_start = start_time
        exec_finish = exec_start + slice_t
        job.remaining -= slice_t
        workers[idx] = exec_finish
        if job.remaining <= 1e-12:
            job.finish = exec_finish
            records.append(job)
        else:
            job.arrival = exec_finish
            pending.append(job)
    all_processed = set(r.request_id for r in records)
    for r in requests:
        if r.request_id not in all_processed:
            idx = min(range(n_workers), key=lambda i: workers[i])
            start = max(r.arrival, workers[idx])
            r.start = start
            r.finish = start + r.service
            workers[idx] = r.finish
            records.append(r)
    records.sort(key=lambda x: x.arrival)
    return records

def write_csv(records, outpath, meta):
    rows = []
    for r in records:
        rows.append({
            "request_id": r.request_id,
            "client_id": r.client_id,
            "arrival_time": float(r.arrival),
            "start_time": float(r.start) if r.start is not None else "",
            "finish_time": float(r.finish) if r.finish is not None else "",
            "service_time": float(r.service),
            **meta
        })
    df = pd.DataFrame(rows)
    df.to_csv(outpath, index=False)

def run_simulation_one(policy, clients, servers, p, quantum, run_idx, results_dir):
    reqs_per_client = 30
    requests = gen_requests(clients, reqs_per_client=reqs_per_client, interarrival_mean=0.02, base_service_mean=0.03, p=p)
    if policy == "fcfs":
        records = simulate_fcfs(requests, servers)
    elif policy == "sjf":
        records = simulate_sjf(requests, servers)
    elif policy == "rr":
        records = simulate_rr(requests, servers, quantum=quantum)
    else:
        raise ValueError("unknown policy")
    fname = f"{policy}_clients{clients}_servers{servers}_p{p}_run{run_idx}.csv"
    outpath = os.path.join(results_dir, fname)
    meta = {"policy": policy, "clients": clients, "servers": servers, "p": p, "run": run_idx}
    write_csv(records, outpath, meta)
    print(f"Wrote {outpath}")
    return outpath

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--simulate", action="store_true", help="run simulator instead of real server/client")
    parser.add_argument("--policies", nargs="+", default=["fcfs","sjf","rr"])
    parser.add_argument("--clients", type=int, default=None)
    parser.add_argument("--server-threads", type=int, default=None)
    parser.add_argument("--p", type=int, default=5)
    parser.add_argument("--quantum", type=float, default=0.1)
    parser.add_argument("--runs", type=int, default=1)
    parser.add_argument("--results-dir", default="results")
    args = parser.parse_args()

    cfg = read_config("config.json")
    clients = args.clients if args.clients is not None else cfg.get("client_threads", 8)
    servers = args.server_threads if args.server_threads is not None else cfg.get("server_threads", 4)

    os.makedirs(args.results_dir, exist_ok=True)

    if args.simulate:
        for policy in args.policies:
            for run in range(args.runs):
                run_simulation_one(policy, clients, servers, args.p, args.quantum, run, args.results_dir)
        print("Simulation complete.")
    else:
        print("Non-simulated run: integrate actual server/client start commands in this script.")
if __name__ == "__main__":
    main()
# [END SCRIPT]
