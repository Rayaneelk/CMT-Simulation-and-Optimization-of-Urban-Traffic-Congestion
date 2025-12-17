#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define INVALID_ID (-1)

typedef enum { DIR_N=0, DIR_E=1, DIR_S=2, DIR_W=3 } Direction;
typedef enum { PHASE_NS=0, PHASE_EW=1 } Phase;
typedef enum { CTRL_FIXED=0, CTRL_ACTUATED=1, CTRL_MAX_PRESSURE=2 } ControllerType;
typedef enum { MOVE_STAY=0, MOVE_WITHIN_LINK=1, MOVE_CROSS=2, MOVE_EXIT=3 } MoveType;

struct Link;
struct Intersection;

typedef struct {
    int vehicle_id; /* -1 if empty */
} Cell;

typedef struct {
    ControllerType type;
    Phase phase;
    double phase_elapsed;

    /* fixed */
    double cycle_time;
    double green_ns;

    /* bounds (actuated/max_pressure) */
    double min_green;
    double max_green;

    /* actuated */
    int queue_threshold;
} TrafficLight;

typedef struct {
    int id;
    int speed;
    int vmax;

    struct Link *link;
    int cell_idx;

    Direction destination_exit;
    double entry_time;
    double stopped_time;
    bool finished;

    MoveType planned_move;
    struct Link *planned_next_link;
    int planned_target_cell;
} Vehicle;

typedef struct Link {
    int id;
    struct Intersection *from; /* NULL => boundary entry */
    struct Intersection *to;   /* NULL => boundary exit  */
    Direction dir;

    int n_cells;
    Cell *cells;
    int stopline_cell;
} Link;

typedef struct Intersection {
    int id;
    int i, j;
    TrafficLight tl;
    Link *in[4];
    Link *out[4];
} Intersection;

#endif
