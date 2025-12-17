// stats.c
#include "stats.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int cmp_double(const void* a, const void* b) {
    double x = *(const double*)a;
    double y = *(const double*)b;
    return (x<y) ? -1 : (x>y);
}

int stats_init(Stats* s, int n_intersections) {
    memset(s, 0, sizeof(*s));
    s->tt_cap = 100000;
    s->travel_times = (double*)malloc(sizeof(double) * (size_t)s->tt_cap);
    s->queue_sum = (double*)calloc((size_t)n_intersections, sizeof(double));
    s->queue_max = (double*)calloc((size_t)n_intersections, sizeof(double));
    return (s->travel_times && s->queue_sum && s->queue_max) ? 0 : -1;
}

void stats_free(Stats* s) {
    free(s->travel_times);
    free(s->queue_sum);
    free(s->queue_max);
    memset(s, 0, sizeof(*s));
}

void stats_on_exit(Stats* s, double travel_time) {
    if (s->tt_n >= s->tt_cap) return;
    s->travel_times[s->tt_n++] = travel_time;
    s->exited++;
}

void stats_collect_queues(Stats* s, const Grid* g) {
    for (int k=0; k<g->n_intersections; k++) {
        const Intersection* inter = &g->intersections[k];
        int q = 0;
        for (int d=0; d<4; d++) {
            const Link* in = inter->in[d];
            if (!in) continue;
            /* queue = occupied cells in last 5 cells */
            int start = in->n_cells - 5;
            if (start < 0) start = 0;
            for (int c=start; c<in->n_cells; c++) if (in->cells[c].vehicle_id != INVALID_ID) q++;
        }
        s->queue_sum[k] += (double)q;
        if ((double)q > s->queue_max[k]) s->queue_max[k] = (double)q;
    }
    s->queue_samples++;
}

void stats_finalize(Stats* s, double measured_time_s) {
    if (s->tt_n > 0) {
        double sum = 0.0;
        for (int i=0;i<s->tt_n;i++) sum += s->travel_times[i];
        s->mean_travel_time_s = sum / (double)s->tt_n;

        /* p95 */
        qsort(s->travel_times, (size_t)s->tt_n, sizeof(double), cmp_double);
        int idx = (int)(0.95 * (double)(s->tt_n - 1));
        if (idx < 0) idx = 0;
        s->p95_travel_time_s = s->travel_times[idx];
    }

    s->throughput_veh_per_s = (measured_time_s > 0.0) ? ((double)s->exited / measured_time_s) : 0.0;

    /* network queue stats */
    double sum_avg = 0.0;
    double maxq = 0.0;
    if (s->queue_samples > 0) {
        for (int k=0; ; k++) { /* break by sentinel via stored arrays elsewhere? handled in export with grid */
            break;
        }
    }
    (void)sum_avg; (void)maxq; /* computed in export using grid size */
}

int stats_export_csv(const Stats* s, const Grid* g, const char* out_dir) {
    char path1[512];
    snprintf(path1, sizeof(path1), "%s/metrics.csv", out_dir);
    FILE* f = fopen(path1, "w");
    if (!f) return -1;

    /* Compute avg/max queue across intersections */
    double avgq_network = 0.0;
    double maxq_network = 0.0;
    if (s->queue_samples > 0) {
        for (int k=0; k<g->n_intersections; k++) {
            double avgk = s->queue_sum[k] / (double)s->queue_samples;
            avgq_network += avgk;
            if (s->queue_max[k] > maxq_network) maxq_network = s->queue_max[k];
        }
        avgq_network /= (double)g->n_intersections;
    }

    fprintf(f, "mean_travel_time_s,p95_travel_time_s,throughput_veh_per_s,avg_queue_veh,max_queue_veh,spawned,exited,blocked_entries\n");
    fprintf(f, "%.6f,%.6f,%.6f,%.6f,%.6f,%ld,%ld,%ld\n",
            s->mean_travel_time_s,
            s->p95_travel_time_s,
            s->throughput_veh_per_s,
            avgq_network,
            maxq_network,
            s->spawned, s->exited, s->blocked_entries);
    fclose(f);

    /* Heatmap */
    char path2[512];
    snprintf(path2, sizeof(path2), "%s/queue_heatmap.csv", out_dir);
    FILE* h = fopen(path2, "w");
    if (!h) return -1;
    fprintf(h, "intersection_id,i,j,avg_queue,max_queue\n");
    for (int k=0; k<g->n_intersections; k++) {
        double avgk = (s->queue_samples > 0) ? (s->queue_sum[k] / (double)s->queue_samples) : 0.0;
        fprintf(h, "%d,%d,%d,%.6f,%.6f\n",
                g->intersections[k].id,
                g->intersections[k].i,
                g->intersections[k].j,
                avgk,
                s->queue_max[k]);
    }
    fclose(h);

    return 0;
}
