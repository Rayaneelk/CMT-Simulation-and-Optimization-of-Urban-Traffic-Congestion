// stats.h
#ifndef STATS_H
#define STATS_H

#include "grid.h"

typedef struct {
    long spawned;
    long exited;
    long blocked_entries;

    /* travel times storage (for p95) */
    double* travel_times;
    int tt_cap;
    int tt_n;

    /* queues per intersection */
    double* queue_sum;
    double* queue_max;
    long queue_samples;

    /* computed */
    double mean_travel_time_s;
    double p95_travel_time_s;
    double throughput_veh_per_s;
    double avg_queue_veh;
    double max_queue_veh;
} Stats;

int stats_init(Stats* s, int n_intersections);
void stats_free(Stats* s);
void stats_on_exit(Stats* s, double travel_time);
void stats_collect_queues(Stats* s, const Grid* g);
void stats_finalize(Stats* s, double measured_time_s);
int stats_export_csv(const Stats* s, const Grid* g, const char* out_dir);

#endif
