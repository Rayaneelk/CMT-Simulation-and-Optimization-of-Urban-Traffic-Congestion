#include "config_kv.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static ControllerType parse_controller(const char* s) {
    if (strcmp(s, "fixed") == 0) return CTRL_FIXED;
    if (strcmp(s, "actuated") == 0) return CTRL_ACTUATED;
    if (strcmp(s, "max_pressure") == 0) return CTRL_MAX_PRESSURE;
    return CTRL_FIXED;
}

static void set_defaults(Config* c) {
    c->time_step = 0.5;
    c->duration = 7200;
    c->warmup = 1200;
    c->random_seed = 42;

    c->grid_size = 6;
    c->cell_length_m = 7.5;
    c->link_length_cells = 20;
    c->lanes_per_direction = 1;

    c->vmax_cells_per_step = 1;
    c->slowdown_probability = 0.2;
    c->vehicle_length_cells = 1;

    c->arrival_rate = 0.25;
    c->routing_randomness = 0.1;

    c->controller = CTRL_FIXED;
    c->cycle_time = 60;
    c->green_ns = 30;

    c->act_min_green = 10;
    c->act_max_green = 40;
    c->act_queue_threshold = 5;

    c->mp_min_green = 5;
    c->mp_max_green = 45;

    c->export_interval = 1.0;
    c->save_queue_snapshots = 1;
    c->save_vehicle_trajectories = 0;
}

int config_load_kv(Config* cfg, const char* path) {
    set_defaults(cfg);

    FILE* f = fopen(path, "r");
    if (!f) return -1;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char* key = line;
        char* val = eq + 1;

        /* trim newline */
        char* nl = strchr(val, '\n');
        if (nl) *nl = '\0';

        if (strcmp(key, "simulation.time_step")==0) cfg->time_step = atof(val);
        else if (strcmp(key, "simulation.duration")==0) cfg->duration = atof(val);
        else if (strcmp(key, "simulation.warmup")==0) cfg->warmup = atof(val);
        else if (strcmp(key, "simulation.random_seed")==0) cfg->random_seed = (uint64_t)strtoull(val, NULL, 10);

        else if (strcmp(key, "network.grid_size")==0) cfg->grid_size = atoi(val);
        else if (strcmp(key, "network.cell_length")==0) cfg->cell_length_m = atof(val);
        else if (strcmp(key, "network.link_length_cells")==0) cfg->link_length_cells = atoi(val);
        else if (strcmp(key, "network.lanes_per_direction")==0) cfg->lanes_per_direction = atoi(val);

        else if (strcmp(key, "vehicles.vmax_cells_per_step")==0) cfg->vmax_cells_per_step = atoi(val);
        else if (strcmp(key, "vehicles.slowdown_probability")==0) cfg->slowdown_probability = atof(val);
        else if (strcmp(key, "vehicles.vehicle_length_cells")==0) cfg->vehicle_length_cells = atoi(val);

        else if (strcmp(key, "demand.arrival_rate")==0) cfg->arrival_rate = atof(val);
        else if (strcmp(key, "demand.routing_randomness")==0) cfg->routing_randomness = atof(val);

        else if (strcmp(key, "traffic_lights.controller")==0) cfg->controller = parse_controller(val);

        else if (strcmp(key, "traffic_lights.fixed.cycle_time")==0) cfg->cycle_time = atof(val);
        else if (strcmp(key, "traffic_lights.fixed.green_ns")==0) cfg->green_ns = atof(val);

        else if (strcmp(key, "traffic_lights.actuated.min_green")==0) cfg->act_min_green = atof(val);
        else if (strcmp(key, "traffic_lights.actuated.max_green")==0) cfg->act_max_green = atof(val);
        else if (strcmp(key, "traffic_lights.actuated.queue_threshold")==0) cfg->act_queue_threshold = atoi(val);

        else if (strcmp(key, "traffic_lights.max_pressure.min_green")==0) cfg->mp_min_green = atof(val);
        else if (strcmp(key, "traffic_lights.max_pressure.max_green")==0) cfg->mp_max_green = atof(val);

        else if (strcmp(key, "output.export_interval")==0) cfg->export_interval = atof(val);
        else if (strcmp(key, "output.save_queue_snapshots")==0) cfg->save_queue_snapshots = atoi(val);
        else if (strcmp(key, "output.save_vehicle_trajectories")==0) cfg->save_vehicle_trajectories = atoi(val);
    }

    fclose(f);
    return 0;
}
