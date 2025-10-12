#!/usr/bin/env python3
import pandas as pd
from matplotlib import pyplot as plt
import os

# ============================================
# Configuration
# ============================================

CSV_FILE = "complete_data.csv"   # ← Input data file
PLOTS_DIR = "plots"
os.makedirs(PLOTS_DIR, exist_ok=True)

figure_count = 1


# ============================================
# Step 1: Load and Aggregate Data
# ============================================

def load_data(csv_file):
    df = pd.read_csv(csv_file, skipinitialspace=True)

    # Ensure numeric types for time fields
    for col in ['arrival_time', 'completion_time', 'response_time', 'waiting_time']:
        df[col] = pd.to_numeric(df[col], errors='coerce')

    # Aggregate data
    grouped = df.groupby(['sched_policy', 'server_threads', 'client_threads', 'packet_size'])
    agg_df = grouped.agg(
        mean_response_time=('response_time', 'mean'),
        median_response_time=('response_time', 'median'),
        mean_waiting_time=('waiting_time', 'mean'),
        num_requests=('req_id', 'count'),
        min_send=('arrival_time', 'min'),
        max_complete=('completion_time', 'max')
    ).reset_index()

    # Compute throughput (requests per second)
    agg_df['throughput'] = agg_df.apply(
        lambda row: row['num_requests'] / ((row['max_complete'] - row['min_send']) / 1000)
        if (row['max_complete'] - row['min_send']) > 0 else 0,
        axis=1
    )

    return agg_df


# ============================================
# Step 2: Generic Plot Function
# ============================================

def generate_graph_generic(df, x_axis, x_field, y_field, variant_field, jitter_value=0.0):
    """
    Automatically determines the two fixed variables (those not used as x_field or variant_field).
    Filters by their first available values and shows them in captions and filenames.
    """
    global figure_count

    all_fields = ['sched_policy', 'server_threads', 'client_threads', 'packet_size']
    fixed_fields = [f for f in all_fields if f not in [x_field, variant_field]]

    # Automatically pick one representative value for each fixed variable
    fixed_values = {f: sorted(df[f].unique())[0] for f in fixed_fields}

    # Filter dataset for fixed values
    sub_df = df.copy()
    for f, val in fixed_values.items():
        sub_df = sub_df[sub_df[f] == val]

    if sub_df.empty:
        print(f"⚠️ Skipping plot: no data for {fixed_values}")
        return

    plt.figure(constrained_layout=True)
    y_axis = {}

    # Build lines per variant
    for _, row in sub_df.iterrows():
        variant_val = str(row[variant_field])
        if variant_val not in y_axis:
            y_axis[variant_val] = [None] * len(x_axis)

        x_val = str(row[x_field])
        if x_val in map(str, x_axis):
            idx = list(map(str, x_axis)).index(x_val)
            y_val = row[y_field]
            y_axis[variant_val][idx] = y_val + list(y_axis).index(variant_val) * jitter_value

    # Plot lines
    for label, values in y_axis.items():
        plt.plot(x_axis, values, marker='o', label=f"{variant_field}={label}")

    plt.xlabel(x_field)
    plt.ylabel(y_field.replace('_', ' ').title())
    plt.grid(True)
    plt.legend(loc="center left", bbox_to_anchor=(1, 0.5))

    # Caption and file name
    fixed_desc = ", ".join([f"{k}={v}" for k, v in fixed_values.items()])
    desc = (f"Figure {figure_count}: {x_field} vs {y_field} for different {variant_field} "
            f"(fixed {fixed_desc})")

    plt.figtext(0.1, 0.02, desc, fontsize=9, ha='left', wrap=True)
    plt.tight_layout()
    plt.subplots_adjust(bottom=0.3)

    fixed_tag = "_".join([f"{k}{v}" for k, v in fixed_values.items()])
    out_file = os.path.join(
        PLOTS_DIR,
        f"{x_field}_vs_{y_field}_diff_{variant_field}_fixed_{fixed_tag}.png"
    )

    plt.savefig(out_file)
    plt.close()

    print(f"✅ Saved plot: {out_file}")
    figure_count += 1


# ============================================
# Step 3: Generate All Plots
# ============================================

def main():
    df = load_data(CSV_FILE)

    sched_policies = sorted(df['sched_policy'].unique())
    server_threads = sorted(df['server_threads'].unique())
    client_threads = sorted(df['client_threads'].unique())
    packet_sizes = sorted(df['packet_size'].unique())

    metrics = ['mean_response_time', 'median_response_time', 'mean_waiting_time', 'throughput']

    # Generate all plots
    for y_field in metrics:

        # sched_policy as X-axis
        for variant_field in ['server_threads', 'client_threads', 'packet_size']:
            generate_graph_generic(df, sched_policies, 'sched_policy', y_field, variant_field)

        # server_threads as X-axis
        for variant_field in ['sched_policy', 'client_threads', 'packet_size']:
            generate_graph_generic(df, server_threads, 'server_threads', y_field, variant_field)

        # client_threads as X-axis
        for variant_field in ['sched_policy', 'server_threads', 'packet_size']:
            generate_graph_generic(df, client_threads, 'client_threads', y_field, variant_field)

        # packet_size as X-axis
        for variant_field in ['sched_policy', 'server_threads', 'client_threads']:
            generate_graph_generic(df, packet_sizes, 'packet_size', y_field, variant_field)


if __name__ == "__main__":
    main()

