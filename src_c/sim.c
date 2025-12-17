// sim.c
#include "sim.h"
#include "rng.h"
#include "controllers.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static VehiclePool vp_init(int cap) {
    VehiclePool vp;
    vp.vehicles = (Vehicle*)malloc(sizeof(Vehicle) * (size_t)cap);
    vp.cap = cap;
    vp.n_used = 0;
    for (int i=0;i<cap;i++) {
        vp.vehicles[i].id = i;
        vp.vehicles[i].finished = true; /* means slot free */
    }
    return vp;
}

static void vp_free(VehiclePool* vp) {
    free(vp->vehicles);
    vp->vehicles = NULL;
    vp->cap = vp->n_used = 0;
}

static int vehicle_create(VehiclePool* vp, double entry_time, Direction dest, int vmax) {
    for (int i=0;i<vp->cap;i++) {
        if (vp->vehicles[i].finished) {
            Vehicle* v = &vp->vehicles[i];
            v->finished = false;
            v->speed = 0;
            v->vmax = vmax;
            v->destination_exit = dest;
            v->entry_time = entry_time;
            v->stopped_time = 0.0;
            v->planned_move = MOVE_STAY;
            v->planned_next_link = NULL;
            v->planned_target_cell = 0;
            return i;
        }
    }
    return -1;
}

static Direction opposite_side(Direction entry_dir) {
    /* entry_dir here is direction of travel on entry link (DIR_S means came from north boundary) */
    if (entry_dir == DIR_S) return DIR_S; /* wants to exit south */
    if (entry_dir == DIR_N) return DIR_N; /* exit north */
    if (entry_dir == DIR_E) return DIR_E; /* exit east */
    return DIR_W; /* exit west */
}

static bool link_is_boundary_exit(const Link* link, const Grid* g, Direction dest_exit) {
    (void)g;
    if (!link->to) return true;
    /* If vehicle reached boundary intersection and wants to go out in that direction and there is no out link */
    const Intersection* inter = link->to;
    return (inter->out[dest_exit] == NULL);
}

static Link* choose_out_link_simple(const Vehicle* v, const Intersection* inter, const Grid* g, double rnd) {
    (void)g;
    /* goal: move toward boundary exit side */
    Direction want = v->destination_exit;

    /* with small probability, pick a random available out link to add diversity */
    if (rng_uniform01() < rnd) {
        for (int tries=0; tries<4; tries++) {
            int d = (int)(rng_uniform01() * 4.0);
            Link* out = inter->out[d];
            if (out) return out;
        }
    }

    /* Prefer the direction "want" if exists */
    if (inter->out[want]) return inter->out[want];

    /* Otherwise pick any existing out link (fallback) */
    for (int d=0; d<4; d++) if (inter->out[d]) return inter->out[d];
    return NULL;
}

static bool can_cross_dir(const Intersection* inter, Direction in_dir) {
    Phase ph = inter->tl.phase;
    if (ph == PHASE_NS) return (in_dir == DIR_N || in_dir == DIR_S);
    return (in_dir == DIR_E || in_dir == DIR_W);
}

static void spawn_vehicles(Grid* g, VehiclePool* vp, const Config* cfg, double t, Stats* s) {
    double p = cfg->arrival_rate * cfg->time_step;
    for (int e=0; e<g->n_entry_links; e++) {
        Link* L = g->entry_links[e];
        if (rng_uniform01() < p) {
            if (L->cells[0].vehicle_id == INVALID_ID) {
                int id = vehicle_create(vp, t, opposite_side(L->dir), cfg->vmax_cells_per_step);
                if (id >= 0) {
                    Vehicle* v = &vp->vehicles[id];
                    v->link = L;
                    v->cell_idx = 0;
                    L->cells[0].vehicle_id = id;
                    s->spawned++;
                }
            } else {
                s->blocked_entries++;
            }
        }
    }
}

static int distance_to_next_vehicle(const Link* L, int cell_idx) {
    int d = 0;
    for (int c=cell_idx+1; c<L->n_cells; c++) {
        if (L->cells[c].vehicle_id != INVALID_ID) break;
        d++;
    }
    return d;
}

static void plan_moves(Grid* g, VehiclePool* vp, const Config* cfg) {
    (void)g;
    for (int i=0;i<vp->cap;i++) {
        Vehicle* v = &vp->vehicles[i];
        if (v->finished) continue;

        Link* L = v->link;
        int vmax = v->vmax;

        /* 1) accel */
        int sp = v->speed + 1;
        if (sp > vmax) sp = vmax;

        /* 2) obstacle distance */
        int gap = distance_to_next_vehicle(L, v->cell_idx);

        /* 3) stopline / intersection handling if would reach end */
        int dist_to_end = L->stopline_cell - v->cell_idx;
        bool may_reach_inter = (sp > dist_to_end);

        int allowed = gap;
        if (allowed > dist_to_end) allowed = dist_to_end; /* default: can't pass stopline unless crossing */

        v->planned_move = MOVE_STAY;
        v->planned_next_link = NULL;
        v->planned_target_cell = v->cell_idx;

        if (may_reach_inter) {
            Intersection* inter = L->to;
            if (!inter) {
                /* this is already a boundary-exit link (not used here) */
            } else if (!can_cross_dir(inter, L->dir)) {
                /* red: stop at stopline */
                allowed = dist_to_end;
            } else {
                /* green: attempt crossing or exiting */
                if (link_is_boundary_exit(L, g, v->destination_exit)) {
                    /* if at boundary and wants to go out */
                    v->planned_move = MOVE_EXIT;
                    v->planned_target_cell = v->cell_idx; /* unused */
                    v->speed = 0; /* will be removed */
                    continue;
                }
                Link* out = choose_out_link_simple(v, inter, g, cfg->routing_randomness);
                if (!out || out->cells[0].vehicle_id != INVALID_ID) {
                    allowed = dist_to_end; /* blocked downstream => don't cross */
                } else {
                    v->planned_move = MOVE_CROSS;
                    v->planned_next_link = out;
                    v->planned_target_cell = 0;
                }
            }
        }

        /* 4) safety braking within link */
        if (v->planned_move != MOVE_CROSS && v->planned_move != MOVE_EXIT) {
            if (sp > allowed) sp = allowed;
        }

        /* 5) random slowdown */
        if (sp > 0 && rng_uniform01() < cfg->slowdown_probability) sp -= 1;

        v->speed = sp;

        if (v->planned_move == MOVE_CROSS) {
            /* crossing ignores speed since we place at cell 0 of out link (dt captures junction) */
            /* keep as is */
        } else if (v->planned_move != MOVE_EXIT) {
            int target = v->cell_idx + v->speed;
            v->planned_move = (target == v->cell_idx) ? MOVE_STAY : MOVE_WITHIN_LINK;
            v->planned_target_cell = target;
        }
    }
}

static void apply_moves(Grid* g, VehiclePool* vp, const Config* cfg, double t, Stats* s) {
    (void)g; (void)cfg;
    /* simple conflict resolution works well with vmax=1:
       we process per link from downstream to upstream for MOVE_WITHIN_LINK,
       and for MOVE_CROSS we just check cell0 was empty at planning time (still verify).
    */

    /* 1) within-link moves */
    for (int lid=0; lid<g->n_links; lid++) {
        Link* L = &g->links[lid];
        /* move from end to start to avoid overwriting */
        for (int c=L->n_cells-1; c>=0; c--) {
            int vid = L->cells[c].vehicle_id;
            if (vid == INVALID_ID) continue;
            Vehicle* v = &vp->vehicles[vid];
            if (v->planned_move == MOVE_WITHIN_LINK) {
                int tgt = v->planned_target_cell;
                if (tgt >= 0 && tgt < L->n_cells && L->cells[tgt].vehicle_id == INVALID_ID) {
                    L->cells[c].vehicle_id = INVALID_ID;
                    L->cells[tgt].vehicle_id = vid;
                    v->cell_idx = tgt;
                } else {
                    /* blocked */
                }
            }
        }
    }

    /* 2) crossings & exits */
    for (int i=0;i<vp->cap;i++) {
        Vehicle* v = &vp->vehicles[i];
        if (v->finished) continue;

        if (v->planned_move == MOVE_CROSS) {
            Link* in = v->link;
            Link* out = v->planned_next_link;

            if (out && out->cells[0].vehicle_id == INVALID_ID) {
                /* remove from in link (must be at stopline cell) */
                int c = v->cell_idx;
                in->cells[c].vehicle_id = INVALID_ID;

                out->cells[0].vehicle_id = v->id;
                v->link = out;
                v->cell_idx = 0;
            }
        } else if (v->planned_move == MOVE_EXIT) {
            double tt = t - v->entry_time;
            if (t >= cfg->warmup) stats_on_exit(s, tt);

            /* remove from cell */
            v->link->cells[v->cell_idx].vehicle_id = INVALID_ID;
            v->finished = true;
        } else {
            if (v->speed == 0) v->stopped_time += cfg->time_step;
        }
    }
}

int sim_run(const Config* cfg, const char* out_dir) {
    Grid g;
    if (grid_init(&g, cfg) != 0) return -1;

    rng_seed(cfg->random_seed);

    /* cap: large enough for 2h simulation; can tune */
    VehiclePool vp = vp_init(200000);

    Stats s;
    if (stats_init(&s, g.n_intersections) != 0) return -1;

    double t = 0.0;
    double dt = cfg->time_step;

    while (t < cfg->duration) {
        spawn_vehicles(&g, &vp, cfg, t, &s);
        update_traffic_lights(&g, cfg, t, dt);

        plan_moves(&g, &vp, cfg);
        apply_moves(&g, &vp, cfg, t, &s);

        if (t >= cfg->warmup) stats_collect_queues(&s, &g);

        t += dt;
    }

    double measured_time = cfg->duration - cfg->warmup;
    if (measured_time < 0) measured_time = 0;

    /* finalize scalar metrics (mean/p95 already computed via array on exit) */
    if (s.tt_n > 0) {
        double sum = 0.0;
        for (int i=0;i<s.tt_n;i++) sum += s.travel_times[i];
        s.mean_travel_time_s = sum / (double)s.tt_n;

        /* compute p95 in-place */
        /* (stats.c sorts inside finalize normally; here quick and simple) */
        /* We'll reuse stats_finalize approach by sorting here: */
        /* to avoid code duplication, simplest: call stats_finalize with measured time and keep export computing throughput separately */
    }
    /* compute p95 + throughput */
    /* sort for p95 */
    if (s.tt_n > 1) {
        /* qsort in stats.c, but keep it simple: call stats_finalize-like logic here is acceptable */
        extern int cmp_double(const void*, const void*); /* not visible; so we keep p95 already approx by export? */
    }

    /* We compute throughput directly */
    s.throughput_veh_per_s = (measured_time > 0.0) ? ((double)s.exited / measured_time) : 0.0;

    /* p95: compute by sorting locally */
    if (s.tt_n > 0) {
        /* local qsort */
        int cmpd(const void* a, const void* b) {
            double x = *(const double*)a;
            double y = *(const double*)b;
            return (x<y) ? -1 : (x>y);
        }
        qsort(s.travel_times, (size_t)s.tt_n, sizeof(double), cmpd);
        int idx = (int)(0.95 * (double)(s.tt_n - 1));
        if (idx < 0) idx = 0;
        s.p95_travel_time_s = s.travel_times[idx];
    }

    /* Ensure out_dir exists (created by Python), then export */
    if (stats_export_csv(&s, &g, out_dir) != 0) return -1;

    stats_free(&s);
    vp_free(&vp);
    grid_free(&g);
    return 0;
}
