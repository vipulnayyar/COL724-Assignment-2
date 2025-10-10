#!/usr/bin/env python3
import os, glob
import pandas as pd
import plotly.express as px
import numpy as np

RESULTS_DIR = "results"
OUT_DIR = os.path.join(RESULTS_DIR, "plots")
os.makedirs(OUT_DIR, exist_ok=True)

def load_all_csvs(results_dir=RESULTS_DIR):
    files = glob.glob(os.path.join(results_dir, "*.csv"))
    dfs = []
    for f in files:
        try:
            df = pd.read_csv(f)
            df["__file"] = os.path.basename(f)
            dfs.append(df)
        except Exception as e:
            print("skip", f, e)
    if not dfs:
        print("No CSVs found in", results_dir)
        return None
    return pd.concat(dfs, ignore_index=True)

def per_file_plots(df):
    grouped = df.groupby("__file")
    for name, g in grouped:
        g = g.sort_values("arrival_time")
        g["response_time"] = g["finish_time"] - g["arrival_time"]
        g["waiting_time"] = g["start_time"] - g["arrival_time"]
        fig = px.line(g, x="request_id", y="response_time", title=f"Response times - {name}")
        fig.write_image(os.path.join(OUT_DIR, f"{name}_response.png"))
        fig2 = px.line(g, x="request_id", y="waiting_time", title=f"Waiting times - {name}")
        fig2.write_image(os.path.join(OUT_DIR, f"{name}_waiting.png"))
        print("Wrote plots for", name)

def summary_plots(df):
    df["response_time"] = df["finish_time"] - df["arrival_time"]
    df["waiting_time"] = df["start_time"] - df["arrival_time"]
    if "policy" in df.columns and "clients" in df.columns:
        agg = df.groupby(["policy","clients","servers","p","run"]).agg(
            mean_response=("response_time","mean"),
            median_response=("response_time","median"),
            mean_wait=("waiting_time","mean"),
            throughput=("request_id", lambda x: x.count() / (df.loc[x.index,"finish_time"].max() - df.loc[x.index,"arrival_time"].min() + 1e-9))
        ).reset_index()
        summary = agg.groupby("policy").mean().reset_index()
        fig = px.bar(summary, x="policy", y="mean_response", title="Mean response time by policy", labels={"mean_response":"Mean response (s)"})
        fig.write_image(os.path.join(OUT_DIR, "mean_response_by_policy.png"))
        print("Wrote mean_response_by_policy.png")
        pivot = agg.groupby(["policy","clients"]).mean().reset_index()
        fig2 = px.line(pivot, x="clients", y="throughput", color="policy", markers=True, title="Throughput vs clients")
        fig2.write_image(os.path.join(OUT_DIR, "throughput_vs_clients.png"))
        print("Wrote throughput_vs_clients.png")
    else:
        print("CSV files lack expected columns 'policy' or 'clients' — ensure simulation uses naming convention.")

def main():
    df = load_all_csvs()
    if df is None:
        return
    per_file_plots(df)
    summary_plots(df)
    print("All plots saved to", OUT_DIR)

if __name__ == "__main__":
    main()
