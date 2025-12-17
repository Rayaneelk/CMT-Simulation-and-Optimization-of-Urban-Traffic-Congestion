from __future__ import annotations
import os
import yaml

def yaml_to_kv(yaml_path: str, kv_out_path: str, overrides: dict | None = None) -> None:
    """
    Reads YAML config and writes a flat key=value config that the C code can parse.
    overrides: optional dict like {"demand.arrival_rate": 0.2, "traffic_lights.controller": "fixed"}
    """
    with open(yaml_path, "r", encoding="utf-8") as f:
        cfg = yaml.safe_load(f)

    # Apply overrides (dot-path keys)
    if overrides:
        for k, v in overrides.items():
            parts = k.split(".")
            cur = cfg
            for p in parts[:-1]:
                cur = cur[p]
            cur[parts[-1]] = v

    # Flatten only what C needs
    sim = cfg["simulation"]
    net = cfg["network"]
    veh = cfg["vehicles"]
    dem = cfg["demand"]
    tl  = cfg["traffic_lights"]
    out = cfg["output"]

    # Controller string -> keep in kv
    controller = tl["controller"]

    lines = []
    def add(key, val):
        lines.append(f"{key}={val}")

    add("simulation.time_step", sim["time_step"])
    add("simulation.duration", sim["duration"])
    add("simulation.warmup", sim["warmup"])
    add("simulation.random_seed", sim["random_seed"])

    add("network.grid_size", net["grid_size"])
    add("network.cell_length", net["cell_length"])
    add("network.link_length_cells", net["link_length_cells"])
    add("network.lanes_per_direction", net["lanes_per_direction"])

    add("vehicles.vmax_cells_per_step", veh["vmax_cells_per_step"])
    add("vehicles.slowdown_probability", veh["slowdown_probability"])
    add("vehicles.vehicle_length_cells", veh["vehicle_length_cells"])

    add("demand.arrival_rate", dem["arrival_rate"])
    add("demand.routing_randomness", dem["routing"]["randomness"])

    add("traffic_lights.controller", controller)

    # Fixed
    add("traffic_lights.fixed.cycle_time", tl["fixed"]["cycle_time"])
    add("traffic_lights.fixed.green_ns", tl["fixed"]["green_ns"])

    # Actuated
    add("traffic_lights.actuated.min_green", tl["actuated"]["min_green"])
    add("traffic_lights.actuated.max_green", tl["actuated"]["max_green"])
    add("traffic_lights.actuated.queue_threshold", tl["actuated"]["queue_threshold"])

    # Max-pressure
    add("traffic_lights.max_pressure.min_green", tl["max_pressure"]["min_green"])
    add("traffic_lights.max_pressure.max_green", tl["max_pressure"]["max_green"])

    add("output.export_interval", out["export_interval"])
    add("output.save_queue_snapshots", int(bool(out["save_queue_snapshots"])))
    add("output.save_vehicle_trajectories", int(bool(out["save_vehicle_trajectories"])))

    os.makedirs(os.path.dirname(kv_out_path), exist_ok=True)
    with open(kv_out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
