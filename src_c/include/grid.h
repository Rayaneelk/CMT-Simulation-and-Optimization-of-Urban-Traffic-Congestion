// grid.h
#ifndef GRID_H
#define GRID_H

#include "config_kv.h"

typedef struct {
    int grid_size;
    int n_intersections;
    int n_links;

    Intersection* intersections;
    Link* links;

    /* boundary entry links list */
    Link** entry_links;
    int n_entry_links;

} Grid;

int grid_init(Grid* g, const Config* cfg);
void grid_free(Grid* g);

#endif
