from __future__ import annotations
import os
import pandas as pd
import matplotlib.pyplot as plt


def plot_curves(summary: pd.DataFrame, out_dir: str) -> None:
    os.makedirs(out_dir, exist_ok=True)

    # Convert lambda from veh/s to veh/min for readability
    summary = summary.copy()
    summary["lambda_per_min"] = summary["lambda"] * 60.0

    plots = [
        ("mean_tt_mean", "mean_travel_time_vs_lambda.png", "Mean travel time [s]"),
        ("p95_tt_mean", "p95_travel_time_vs_lambda.png", "P95 travel time [s]"),
        ("throughput_mean", "throughput_vs_lambda.png", "Throughput [veh/s]"),
        ("avg_queue_mean", "avg_queue_vs_lambda.png", "Average queue [veh]"),
        ("blocked_entries_mean", "blocked_entries_vs_lambda.png", "Blocked entries [count]"),
    ]

    for ycol, fname, ylabel in plots:
        if ycol not in summary.columns:
            continue

        plt.figure()
        for ctrl, sub in summary.groupby("controller"):
            sub = sub.sort_values("lambda_per_min")
            plt.plot(sub["lambda_per_min"], sub[ycol], label=ctrl)

        plt.xlabel("Arrival rate λ [veh/min per entry]")
        plt.ylabel(ylabel)
        plt.legend()
        plt.tight_layout()
        plt.savefig(os.path.join(out_dir, fname), dpi=200)
        plt.close()

