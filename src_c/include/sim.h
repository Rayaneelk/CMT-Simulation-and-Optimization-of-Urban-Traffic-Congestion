// sim.h
#ifndef SIM_H
#define SIM_H
#include "stats.h"

typedef struct {
    Vehicle* vehicles;
    int cap;
    int n_used;
} VehiclePool;

int sim_run(const Config* cfg, const char* out_dir);

#endif
