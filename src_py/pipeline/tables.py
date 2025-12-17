from __future__ import annotations
import os
import pandas as pd

def save_summary_table(summary: pd.DataFrame, out_dir: str) -> None:
    os.makedirs(out_dir, exist_ok=True)
    summary_sorted = summary.sort_values(["lambda", "controller"])
    summary_sorted.to_csv(os.path.join(out_dir, "summary.csv"), index=False)
