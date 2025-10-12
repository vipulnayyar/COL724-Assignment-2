#!/usr/bin/env python3
import pandas as pd

# Input and output files
INPUT_CSV = "combined.csv"
OUTPUT_CSV = "complete_data.csv"

# Column names for the original data
COLUMN_NAMES = [
    "req_id",
    "req_type",
    "file_path",
    "sched_policy",
    "server_threads",
    "client_threads",
    "packet_size",
    "arrival_time",
    "service_start_time",
    "completion_time"
]

def add_response_waiting_times(input_file, output_file):
    # Read CSV, skip first line, assign column names, ignore extra columns
    df = pd.read_csv(
        input_file,
        header=None,
        skiprows=1,
        names=COLUMN_NAMES,
        usecols=range(len(COLUMN_NAMES))
    )

    # Convert times to numeric
    for col in ["arrival_time", "service_start_time", "completion_time"]:
        df[col] = pd.to_numeric(df[col], errors='coerce')

    # Compute response_time = completion_time - service_start_time
    df["response_time"] = df["completion_time"] - df["service_start_time"]

    # Compute waiting_time = service_start_time - arrival_time
    df["waiting_time"] = df["service_start_time"] - df["arrival_time"]

    # Save the processed data to CSV
    df.to_csv(output_file, index=False)
    print(f"✅ Processed file saved as {output_file}")

if __name__ == "__main__":
    add_response_waiting_times(INPUT_CSV, OUTPUT_CSV)

