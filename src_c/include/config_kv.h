#ifndef CONFIG_KV_H
#define CONFIG_KV_H

#include "sim_types.h"

typedef struct {
    /* simulation */
    double time_step;
    double duration;
    double warmup;
    uint64_t random_seed;

    /* network */
    int grid_size;
    double cell_length_m;
    int link_length_cells;
    int lanes_per_direction;

    /* vehicles */
    int vmax_cells_per_step;
    double slowdown_probability;
    int vehicle_length_cells;

    /* demand */
    double arrival_rate;
    double routing_randomness;

    /* traffic lights */
    ControllerType controller;

    /* fixed */
    double cycle_time;
    double green_ns;

    /* actuated */
    double act_min_green;
    double act_max_green;
    int act_queue_threshold;

    /* max pressure */
    double mp_min_green;
    double mp_max_green;

    /* output */
    double export_interval;
    int save_queue_snapshots;
    int save_vehicle_trajectories;

} Config;

int config_load_kv(Config* cfg, const char* path);

#endif
