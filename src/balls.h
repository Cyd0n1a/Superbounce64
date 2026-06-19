#ifndef BALLS_H
#define BALLS_H

#include <stdint.h>

typedef struct {
    float    x, y;
    float    vx, vy;
    uint8_t  color_idx;
} Ball;

extern Ball  balls[8];
extern int   num_balls;

void balls_init(int count, float speed);
void balls_update(float dt);

#endif
