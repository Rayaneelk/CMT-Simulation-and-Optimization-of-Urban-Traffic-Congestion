from __future__ import annotations
import os, re
import pandas as pd

PAT = re.compile(r"lam_(?P<lam>\d+\.\d+)_ctrl_(?P<ctrl>[a-z_]+)_seed_(?P<seed>\d+)")

def aggregate_results(results_root: str) -> pd.DataFrame:
    rows = []
    for name in os.listdir(results_root):
        m = PAT.match(name)
        if not m:
            continue
        lam = float(m.group("lam"))
        ctrl = m.group("ctrl")
        seed = int(m.group("seed"))

        metrics_path = os.path.join(results_root, name, "metrics.csv")
        if not os.path.exists(metrics_path):
            continue
        met = pd.read_csv(metrics_path).iloc[0].to_dict()
        met.update({"lambda": lam, "controller": ctrl, "seed": seed})
        rows.append(met)

    df = pd.DataFrame(rows)
    return df

def summarize(df: pd.DataFrame) -> pd.DataFrame:
    g = df.groupby(["lambda", "controller"])
    agg = g.agg(
        mean_tt_mean=("mean_travel_time_s", "mean"),
        mean_tt_std=("mean_travel_time_s", "std"),
        p95_tt_mean=("p95_travel_time_s", "mean"),
        throughput_mean=("throughput_veh_per_s", "mean"),
        avg_queue_mean=("avg_queue_veh", "mean"),
        max_queue_mean=("max_queue_veh", "mean"),
        blocked_entries_mean=("blocked_entries", "mean"),
    ).reset_index()
    return agg
