from __future__ import annotations
import os
import argparse
import subprocess
import shutil

from .run_batch import run_sweep
from .aggregate import aggregate_results, summarize
from .plots import plot_curves
from .tables import save_summary_table


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", default="config/base.yaml")
    ap.add_argument("--results", default="results")
    ap.add_argument("--figures", default="report_figures")
    ap.add_argument("--tables", default="report_tables")
    ap.add_argument("--sim_bin", default="src_c/bin/traffic_sim.exe")
    args = ap.parse_args()

    # --------------------------------------------------
    # Build C simulator if needed (Windows / MSYS2 safe)
    # --------------------------------------------------
    if not os.path.exists(args.sim_bin):
        make_cmd = shutil.which("mingw32-make") or shutil.which("make")
        if make_cmd is None:
            raise RuntimeError(
                "Neither 'mingw32-make' nor 'make' was found in PATH."
            )
        subprocess.run([make_cmd, "-C", "src_c"], check=True)

    # --------------------------------------------------
    # Experiment design
    # --------------------------------------------------
    # --- Define demand levels in veh/min per entry (client-friendly) ---
    lambdas_per_min = [3, 6, 9, 12, 15, 18, 21, 24, 27, 30]  # integers, per entry
    lambdas = [lm / 60.0 for lm in lambdas_per_min]  # convert to veh/s for the simulator
    controllers = ["fixed", "actuated", "max_pressure"]
    seeds = list(range(5))  # keep small for quick runs; increase later

    run_sweep(
        sim_bin=args.sim_bin,
        yaml_config=args.config,
        results_root=args.results,
        lambdas=lambdas,              # still veh/s internally
        controllers=controllers,
        seeds=seeds,
    )


    # --------------------------------------------------
    # Aggregate + outputs for report
    # --------------------------------------------------
    df = aggregate_results(args.results)
    os.makedirs(args.tables, exist_ok=True)
    os.makedirs(args.figures, exist_ok=True)

    df.to_csv(os.path.join(args.tables, "raw_metrics.csv"), index=False)

    summary = summarize(df)
    save_summary_table(summary, args.tables)
    plot_curves(summary, args.figures)


if __name__ == "__main__":
    main()
