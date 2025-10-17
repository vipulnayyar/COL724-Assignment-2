#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import os

# ============================================
# Configuration
# ============================================

CSV_FILE = "health_data_lc.csv"
PLOTS_DIR = "plots_healthcheck_lc"
os.makedirs(PLOTS_DIR, exist_ok=True)
plt.rcParams.update({'figure.figsize': (8, 4), 'font.size': 10})


# ============================================
# Step 1: Load and Prepare Data
# ============================================

def load_health_data(csv_file):
    cols = [
        "timestamp", "server_id", "server_ip", "server_port",
        "status", "response_time_ms", "queue_length",
        "connection_throughput", "average_time_spent_on_server"
    ]
    df = pd.read_csv(csv_file, names=cols)

    df["timestamp"] = pd.to_datetime(df["timestamp"], unit="ms")
    numeric_cols = [
        "server_id", "status", "response_time_ms", "queue_length",
        "connection_throughput", "average_time_spent_on_server"
    ]
    for c in numeric_cols:
        df[c] = pd.to_numeric(df[c], errors="coerce")

    return df


# ============================================
# Step 2: Plot Helpers
# ============================================

def plot_time_series(df, y_field, ylabel):
    """Plot Y vs timestamp for each server."""
    plt.figure()
    for sid, group in df.groupby("server_id"):
        plt.plot(group["timestamp"], group[y_field], marker=".", label=f"Server {sid}")
    plt.xlabel("Time")
    plt.ylabel(ylabel)
    plt.title(f"{ylabel} over Time (per Server)")
    plt.grid(True)
    plt.legend()
    out_path = os.path.join(PLOTS_DIR, f"{y_field}_over_time.png")
    plt.savefig(out_path, bbox_inches="tight")
    plt.close()
    print(f"✅ Saved: {out_path}")


def plot_bar(df, y_field, ylabel):
    """Plot per-server average metric."""
    agg = df.groupby("server_id")[y_field].mean().reset_index()
    plt.figure()
    plt.bar(agg["server_id"], agg[y_field])
    plt.xlabel("Server ID")
    plt.ylabel(ylabel)
    plt.title(f"Average {ylabel} per Server")
    plt.grid(axis="y", linestyle="--", alpha=0.7)
    out_path = os.path.join(PLOTS_DIR, f"avg_{y_field}_per_server.png")
    plt.savefig(out_path, bbox_inches="tight")
    plt.close()
    print(f"✅ Saved: {out_path}")


def plot_correlation(df):
    """Scatter: Response Time vs Avg Time Spent"""
    plt.figure()
    for sid, group in df.groupby("server_id"):
        plt.scatter(group["average_time_spent_on_server"],
                    group["response_time_ms"], alpha=0.6, label=f"Server {sid}")
    plt.xlabel("Average Time Spent on Server (ms)")
    plt.ylabel("Response Time (ms)")
    plt.title("Correlation: Avg Service Time vs Response Time")
    plt.grid(True)
    plt.legend()
    out_path = os.path.join(PLOTS_DIR, "corr_response_vs_service.png")
    plt.savefig(out_path, bbox_inches="tight")
    plt.close()
    print(f"✅ Saved: {out_path}")


# ============================================
# Step 3: Main
# ============================================

def main():
    df = load_health_data(CSV_FILE)

    # Only include healthy responses
    healthy_df = df[df["status"] == 1]

    # Time-series plots
    plot_time_series(healthy_df, "response_time_ms", "Response Time (ms)")
    plot_time_series(healthy_df, "queue_length", "Queue Length")
    plot_time_series(healthy_df, "connection_throughput", "Connection Throughput (req/s)")
    plot_time_series(healthy_df, "average_time_spent_on_server", "Avg Time Spent on Server (ms)")

    # Aggregate comparison plots
    plot_bar(healthy_df, "response_time_ms", "Response Time (ms)")
    plot_bar(healthy_df, "queue_length", "Queue Length")
    plot_bar(healthy_df, "connection_throughput", "Connection Throughput (req/s)")

    # Correlation plot
    plot_correlation(healthy_df)

    print("✅ All plots generated in:", PLOTS_DIR)


if __name__ == "__main__":
    main()
