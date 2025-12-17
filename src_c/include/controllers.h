// controllers.h
#ifndef CONTROLLERS_H
#define CONTROLLERS_H
#include "grid.h"

void update_traffic_lights(Grid* g, const Config* cfg, double t, double dt);

/* helpers for queues/pressure */
int queue_in_dir(const Intersection* inter, Direction dir, int k_cells);
int pressure_for_phase(const Intersection* inter, Phase ph, int k_cells);

#endif
