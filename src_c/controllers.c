// controllers.c
#include "controllers.h"
#include <math.h>

static bool is_green_for_dir(Phase ph, Direction dir) {
    /* NS green allows DIR_N and DIR_S movements into the intersection */
    if (ph == PHASE_NS) return (dir == DIR_N || dir == DIR_S);
    return (dir == DIR_E || dir == DIR_W);
}

int queue_in_dir(const Intersection* inter, Direction dir, int k_cells) {
    const Link* in = inter->in[dir];
    if (!in) return 0;
    int count = 0;
    int start = in->n_cells - k_cells;
    if (start < 0) start = 0;
    for (int c=start; c<in->n_cells; c++) {
        if (in->cells[c].vehicle_id != INVALID_ID) count++;
    }
    return count;
}

int pressure_for_phase(const Intersection* inter, Phase ph, int k_cells) {
    /* upstream queues on approaches that would be green */
    int upstream = 0;
    int downstream = 0;

    for (int d=0; d<4; d++) {
        Direction dir = (Direction)d;
        if (!is_green_for_dir(ph, dir)) continue;

        upstream += queue_in_dir(inter, dir, k_cells);

        /* downstream: approximate congestion on outgoing link of same dir (straight movement) */
        const Link* out = inter->out[dir];
        if (!out) continue;
        int start = 0;
        int end = k_cells;
        if (end > out->n_cells) end = out->n_cells;
        for (int c=start; c<end; c++) {
            if (out->cells[c].vehicle_id != INVALID_ID) downstream++;
        }
    }
    return upstream - downstream;
}

static void tl_fixed(TrafficLight* tl, double t) {
    double C = tl->cycle_time;
    double gNS = tl->green_ns;
    double x = fmod(t, C);
    tl->phase = (x < gNS) ? PHASE_NS : PHASE_EW;
}

static void tl_actuated(Intersection* inter, TrafficLight* tl, double dt) {
    tl->phase_elapsed += dt;

    int qNS = queue_in_dir(inter, DIR_N, 5) + queue_in_dir(inter, DIR_S, 5);
    int qEW = queue_in_dir(inter, DIR_E, 5) + queue_in_dir(inter, DIR_W, 5);

    if (tl->phase_elapsed < tl->min_green) return;

    bool force = (tl->phase_elapsed >= tl->max_green);
    bool wantNS = (qNS - qEW) > tl->queue_threshold;
    bool wantEW = (qEW - qNS) > tl->queue_threshold;

    if (force || (tl->phase == PHASE_EW && wantNS) || (tl->phase == PHASE_NS && wantEW)) {
        tl->phase = (tl->phase == PHASE_NS) ? PHASE_EW : PHASE_NS;
        tl->phase_elapsed = 0.0;
    }
}

static void tl_max_pressure(Intersection* inter, TrafficLight* tl, double dt) {
    tl->phase_elapsed += dt;

    if (tl->phase_elapsed < tl->min_green) return;

    int pNS = pressure_for_phase(inter, PHASE_NS, 5);
    int pEW = pressure_for_phase(inter, PHASE_EW, 5);
    Phase best = (pNS >= pEW) ? PHASE_NS : PHASE_EW;

    bool force = (tl->phase_elapsed >= tl->max_green);

    if (force || best != tl->phase) {
        tl->phase = best;
        tl->phase_elapsed = 0.0;
    }
}

void update_traffic_lights(Grid* g, const Config* cfg, double t, double dt) {
    for (int k=0; k<g->n_intersections; k++) {
        Intersection* inter = &g->intersections[k];
        TrafficLight* tl = &inter->tl;
        tl->type = cfg->controller;

        if (cfg->controller == CTRL_FIXED) {
            tl->cycle_time = cfg->cycle_time;
            tl->green_ns = cfg->green_ns;
            tl_fixed(tl, t);
        } else if (cfg->controller == CTRL_ACTUATED) {
            tl->min_green = cfg->act_min_green;
            tl->max_green = cfg->act_max_green;
            tl->queue_threshold = cfg->act_queue_threshold;
            tl_actuated(inter, tl, dt);
        } else {
            tl->min_green = cfg->mp_min_green;
            tl->max_green = cfg->mp_max_green;
            tl_max_pressure(inter, tl, dt);
        }
    }
}
