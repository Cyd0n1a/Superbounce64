#include "balls.h"
#include "field.h"
#include "game.h"
#include "sfx.h"
#include <math.h>

Ball balls[MAX_BALLS];
int  num_balls;

/* Neon palette: cyan, magenta, yellow, green, orange, white, pink, aqua */
static const float ball_colors[8][3] = {
    { 0.0f, 1.0f, 1.0f },
    { 1.0f, 0.0f, 1.0f },
    { 1.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 0.2f },
    { 1.0f, 0.5f, 0.0f },
    { 1.0f, 1.0f, 1.0f },
    { 1.0f, 0.2f, 0.5f },
    { 0.2f, 0.8f, 1.0f },
};

/* Expose color for renderer */
const float (*balls_color_table(void))[3] { return ball_colors; }

void balls_init(int count, float speed) {
    num_balls = count;
    /* Divide playfield into a grid and place one ball per cell.
       This guarantees even spread regardless of ball count. */
    int cols = (count <= 4) ? 2 : 3;
    int rows = (count + cols - 1) / cols;
    float margin = (float)(CELL_SIZE * 6);
    float cw = (SCREEN_W - margin * 2.f) / (float)cols;
    float ch = (SCREEN_H - margin * 2.f) / (float)rows;
    for (int i = 0; i < count; i++) {
        int col = i % cols;
        int row = i / cols;
        balls[i].x = margin + cw * (col + 0.5f);
        balls[i].y = margin + ch * (row + 0.5f);
        /* Evenly spread launch angles so balls don't all go the same way */
        float angle = (float)i * (6.28318f / (float)count) + 0.4f;
        balls[i].vx = cosf(angle) * speed;
        balls[i].vy = sinf(angle) * speed;
        balls[i].color_idx = (uint8_t)(i % 8);
    }
}

void balls_update(float dt) {
    for (int i = 0; i < num_balls; i++) {
        Ball *b = &balls[i];

        float nx = b->x + b->vx * dt;
        float ny = b->y + b->vy * dt;

        int cx  = field_cell_x(b->x);
        int cy  = field_cell_y(b->y);
        int ncx = field_cell_x(nx);
        int ncy = field_cell_y(ny);

        /* Check horizontal movement */
        if (ncx != cx && field_is_solid(ncx, cy)) {
            b->vx = -b->vx;
            nx = b->x;
            ncx = cx;
            sfx_play(SFX_BALL_BOUNCE);
        }
        /* Check vertical movement */
        if (ncy != cy && field_is_solid(ncx, ncy)) {
            b->vy = -b->vy;
            ny = b->y;
            sfx_play(SFX_BALL_BOUNCE);
        }

        b->x = nx;
        b->y = ny;

        /* Clamp to interior (inside border wall cells) */
        if (b->x < CELL_SIZE)             { b->x = CELL_SIZE;             b->vx =  fabsf(b->vx); }
        if (b->x > (GRID_W-2)*CELL_SIZE)  { b->x = (GRID_W-2)*CELL_SIZE; b->vx = -fabsf(b->vx); }
        if (b->y < CELL_SIZE)             { b->y = CELL_SIZE;             b->vy =  fabsf(b->vy); }
        if (b->y > (GRID_H-2)*CELL_SIZE)  { b->y = (GRID_H-2)*CELL_SIZE; b->vy = -fabsf(b->vy); }
    }
}
