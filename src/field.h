#ifndef FIELD_H
#define FIELD_H

#include <stdint.h>
#include "game.h"

#define CELL_EMPTY   0
#define CELL_WALL    1
#define CELL_CLAIMED 2

extern uint8_t field[GRID_H][GRID_W];

void field_init(void);
void field_mark_border(void);

/* Run BFS from ball positions; remaining EMPTY cells become CLAIMED.
   Returns new claimed percentage. */
int field_flood_and_claim(float *ball_xs, float *ball_ys, int num_balls);

int field_claimed_percent(void);

static inline int field_cell_x(float screen_x) { return (int)screen_x / CELL_SIZE; }
static inline int field_cell_y(float screen_y) { return (int)screen_y / CELL_SIZE; }
static inline int field_is_solid(int cx, int cy) {
    if (cx < 0 || cx >= GRID_W || cy < 0 || cy >= GRID_H) return 1;
    return field[cy][cx] != CELL_EMPTY;
}

#endif
