from __future__ import annotations
import os, subprocess, shutil
from .yaml_to_kv import yaml_to_kv

def run_one(sim_bin: str, yaml_config: str, out_dir: str, overrides: dict) -> None:
    os.makedirs(out_dir, exist_ok=True)
    kv_path = os.path.join(out_dir, "config.kv")
    yaml_to_kv(yaml_config, kv_path, overrides=overrides)

    # Run C simulator
    cmd = [sim_bin, "--config", kv_path, "--out", out_dir]
    subprocess.run(cmd, check=True)

def run_sweep(sim_bin: str, yaml_config: str, results_root: str,
              lambdas: list[float], controllers: list[str], seeds: list[int]) -> None:
    if os.path.exists(results_root):
        shutil.rmtree(results_root)
    os.makedirs(results_root, exist_ok=True)

    for lam in lambdas:
        for ctrl in controllers:
            for seed in seeds:
                tag = f"lam_{lam:.3f}_ctrl_{ctrl}_seed_{seed}"
                out_dir = os.path.join(results_root, tag)
                overrides = {
                    "demand.arrival_rate": lam,
                    "traffic_lights.controller": ctrl,
                    "simulation.random_seed": seed
                }
                run_one(sim_bin, yaml_config, out_dir, overrides)
