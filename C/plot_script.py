import pandas as pd
import matplotlib.pyplot as plt
import itertools

# ----------------- Load Data -----------------
ctrl_log = pd.read_csv("link_metrics.csv")
ctrl_log['time'] = pd.to_datetime(ctrl_log['time'], unit='s')

cong_log = pd.read_csv("congestion_log.csv")
cong_log['start'] = pd.to_datetime(cong_log['start'], unit='s')
cong_log['end'] = pd.to_datetime(cong_log['end'], unit='s')

# ----------------- Colors for congestion levels -----------------
level_colors = {2: 'orange', 5: 'red', 8: 'purple'}

# ----------------- Metrics -----------------
metrics = ['throughput_mbps', 'utilization', 'weight', 'delay']

# ----------------- Generate plots -----------------
links = cong_log['link'].unique()

for link in links:
    src, dst = link.split('-')
    # Filter controller log for this link
    link_data = ctrl_log[(ctrl_log['src']==int(src[1:])) & (ctrl_log['dst']==int(dst[1:]))]
    link_cong = cong_log[cong_log['link']==link]

    # All combinations of 2 metrics
    for m1, m2 in itertools.combinations(metrics, 2):
        plt.figure(figsize=(14,6))

        # Plot metrics vs time
        plt.plot(link_data['time'], link_data[m1], label=m1, color='blue')
        plt.plot(link_data['time'], link_data[m2], label=m2, color='green')

        # Highlight congestion periods
        for _, row in link_cong.iterrows():
            plt.axvspan(row['start'], row['end'], color=level_colors.get(row['level'], 'gray'), alpha=0.3,
                        label=f"Congestion Level {row['level']}" if _==0 else "")

        plt.xlabel("Time")
        plt.ylabel(f"{m1} & {m2}")
        plt.title(f"Link {link}: {m1} & {m2} vs Time with Congestion")
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.show()

