// grid.c
#include "grid.h"
#include <stdlib.h>
#include <string.h>

static void init_light(TrafficLight* tl, const Config* cfg) {
    tl->type = cfg->controller;
    tl->phase = PHASE_NS;
    tl->phase_elapsed = 0.0;

    tl->cycle_time = cfg->cycle_time;
    tl->green_ns = cfg->green_ns;

    tl->min_green = (cfg->controller == CTRL_MAX_PRESSURE) ? cfg->mp_min_green : cfg->act_min_green;
    tl->max_green = (cfg->controller == CTRL_MAX_PRESSURE) ? cfg->mp_max_green : cfg->act_max_green;
    tl->queue_threshold = cfg->act_queue_threshold;
}

static Link* new_link(Grid* g, int id, Intersection* from, Intersection* to, Direction dir, int n_cells) {
    Link* L = &g->links[id];
    L->id = id;
    L->from = from;
    L->to = to;
    L->dir = dir;
    L->n_cells = n_cells;
    L->stopline_cell = n_cells - 1;
    L->cells = (Cell*)malloc(sizeof(Cell) * (size_t)n_cells);
    for (int i=0;i<n_cells;i++) L->cells[i].vehicle_id = INVALID_ID;
    return L;
}

int grid_init(Grid* g, const Config* cfg) {
    memset(g, 0, sizeof(*g));
    g->grid_size = cfg->grid_size;
    g->n_intersections = cfg->grid_size * cfg->grid_size;

    g->intersections = (Intersection*)malloc(sizeof(Intersection) * (size_t)g->n_intersections);
    for (int idx=0; idx<g->n_intersections; idx++) {
        Intersection* I = &g->intersections[idx];
        I->id = idx;
        I->i = idx / cfg->grid_size;
        I->j = idx % cfg->grid_size;
        for (int d=0; d<4; d++) { I->in[d]=NULL; I->out[d]=NULL; }
        init_light(&I->tl, cfg);
    }

    /* Links: for each adjacent pair we create 2 directed links (both directions).
       Grid has (N*(N-1)) horizontal adjacencies and same vertical. Each adjacency => 2 links.
       Total internal directed links = 2*(N*(N-1) + N*(N-1)) = 4*N*(N-1).
       Plus boundary entry links: 4*N (entering from each side).
       Plus boundary exit links are represented by internal links having to=NULL; we won't create them separately.
       Simpler: create entry links only (from=NULL to=boundary row/col intersection).
    */
    int N = cfg->grid_size;
    int internal = 4 * N * (N - 1);
    int entries = 4 * N;
    g->n_links = internal + entries;
    g->links = (Link*)malloc(sizeof(Link) * (size_t)g->n_links);

    g->entry_links = (Link**)malloc(sizeof(Link*) * (size_t)entries);
    g->n_entry_links = entries;

    int lid = 0;
    /* internal vertical links */
    for (int i=0;i<N-1;i++) for (int j=0;j<N;j++) {
        Intersection* A = &g->intersections[i*N + j];
        Intersection* B = &g->intersections[(i+1)*N + j];
        /* A -> B is DIR_S (going down) */
        Link* L1 = new_link(g, lid++, A, B, DIR_S, cfg->link_length_cells);
        /* B -> A is DIR_N (going up) */
        Link* L2 = new_link(g, lid++, B, A, DIR_N, cfg->link_length_cells);
        A->out[DIR_S] = L1; B->in[DIR_S] = L1;
        B->out[DIR_N] = L2; A->in[DIR_N] = L2;
    }
    /* internal horizontal links */
    for (int i=0;i<N;i++) for (int j=0;j<N-1;j++) {
        Intersection* A = &g->intersections[i*N + j];
        Intersection* B = &g->intersections[i*N + (j+1)];
        /* A -> B is DIR_E */
        Link* L1 = new_link(g, lid++, A, B, DIR_E, cfg->link_length_cells);
        /* B -> A is DIR_W */
        Link* L2 = new_link(g, lid++, B, A, DIR_W, cfg->link_length_cells);
        A->out[DIR_E] = L1; B->in[DIR_E] = L1;
        B->out[DIR_W] = L2; A->in[DIR_W] = L2;
    }

    /* boundary entry links */
    int eidx = 0;
    /* Enter from North going South into row 0 */
    for (int j=0;j<N;j++) {
        Intersection* to = &g->intersections[0*N + j];
        Link* L = new_link(g, lid++, NULL, to, DIR_S, cfg->link_length_cells);
        to->in[DIR_S] = L;
        g->entry_links[eidx++] = L;
    }
    /* Enter from South going North into row N-1 */
    for (int j=0;j<N;j++) {
        Intersection* to = &g->intersections[(N-1)*N + j];
        Link* L = new_link(g, lid++, NULL, to, DIR_N, cfg->link_length_cells);
        to->in[DIR_N] = L;
        g->entry_links[eidx++] = L;
    }
    /* Enter from West going East into col 0 */
    for (int i=0;i<N;i++) {
        Intersection* to = &g->intersections[i*N + 0];
        Link* L = new_link(g, lid++, NULL, to, DIR_E, cfg->link_length_cells);
        to->in[DIR_E] = L;
        g->entry_links[eidx++] = L;
    }
    /* Enter from East going West into col N-1 */
    for (int i=0;i<N;i++) {
        Intersection* to = &g->intersections[i*N + (N-1)];
        Link* L = new_link(g, lid++, NULL, to, DIR_W, cfg->link_length_cells);
        to->in[DIR_W] = L;
        g->entry_links[eidx++] = L;
    }

    return 0;
}

void grid_free(Grid* g) {
    if (!g) return;
    if (g->links) {
        for (int i=0;i<g->n_links;i++) free(g->links[i].cells);
        free(g->links);
    }
    free(g->intersections);
    free(g->entry_links);
    memset(g, 0, sizeof(*g));
}
