#include "player.h"
#include "field.h"
#include "balls.h"
#include "game.h"
#include "sfx.h"
#include <libdragon.h>
#include <math.h>

float player_x, player_y;

#define CURSOR_SPEED   120.f   /* pixels per second */
#define STICK_DEADZONE  10

void player_init(void) {
    player_x = SCREEN_W / 2.f;
    player_y = SCREEN_H / 2.f;
}

static int balls_in_range(int x0, int y0, int x1, int y1) {
    /* Check if any ball's cell falls within the horizontal or vertical span */
    for (int i = 0; i < num_balls; i++) {
        int bcx = field_cell_x(balls[i].x);
        int bcy = field_cell_y(balls[i].y);
        if (x0 == x1) {
            /* vertical span: x fixed, y varies */
            int ylo = y0 < y1 ? y0 : y1;
            int yhi = y0 < y1 ? y1 : y0;
            if (bcx == x0 && bcy >= ylo && bcy <= yhi) return 1;
        } else {
            /* horizontal span: y fixed, x varies */
            int xlo = x0 < x1 ? x0 : x1;
            int xhi = x0 < x1 ? x1 : x0;
            if (bcy == y0 && bcx >= xlo && bcx <= xhi) return 1;
        }
    }
    return 0;
}

static void place_wall(int cx, int cy, int horizontal) {
    /* Scan outward in both directions until hitting a solid cell */
    int lo, hi;
    if (horizontal) {
        lo = cx; hi = cx;
        while (lo > 0         && !field_is_solid(lo - 1, cy)) lo--;
        while (hi < GRID_W-1  && !field_is_solid(hi + 1, cy)) hi++;
        if (balls_in_range(lo, cy, hi, cy)) {
            g.state = STATE_WALL_FAIL; g.state_timer = 0.5f;
            sfx_play(SFX_WALL_FAIL);
            return;
        }
        for (int x = lo; x <= hi; x++) field[cy][x] = CELL_WALL;
    } else {
        lo = cy; hi = cy;
        while (lo > 0         && !field_is_solid(cx, lo - 1)) lo--;
        while (hi < GRID_H-1  && !field_is_solid(cx, hi + 1)) hi++;
        if (balls_in_range(cx, lo, cx, hi)) {
            g.state = STATE_WALL_FAIL; g.state_timer = 0.5f;
            sfx_play(SFX_WALL_FAIL);
            return;
        }
        for (int y = lo; y <= hi; y++) field[y][cx] = CELL_WALL;
    }

    sfx_play(SFX_WALL_PLACE);

    /* Build ball position arrays for flood fill */
    float bxs[MAX_BALLS], bys[MAX_BALLS];
    for (int i = 0; i < num_balls; i++) { bxs[i] = balls[i].x; bys[i] = balls[i].y; }

    g.claimed_pct = field_flood_and_claim(bxs, bys, num_balls);
    if (g.claimed_pct >= CLAIM_TARGET) {
        g.state = STATE_LEVEL_COMPLETE;
        g.state_timer = 2.0f;
        sfx_play(SFX_LEVEL_COMPLETE);
    }
}

/* If the cursor ended up inside a solid cell (e.g. after flood-fill claimed it),
   walk outward in a spiral until we find the nearest empty cell. */
static void resolve_overlap(void) {
    int cx = field_cell_x(player_x);
    int cy = field_cell_y(player_y);
    if (!field_is_solid(cx, cy)) return;

    for (int r = 1; r < GRID_W; r++) {
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (abs(dx) != r && abs(dy) != r) continue;  /* only the ring */
                int nx = cx + dx, ny = cy + dy;
                if (nx < 1 || nx >= GRID_W-1 || ny < 1 || ny >= GRID_H-1) continue;
                if (!field_is_solid(nx, ny)) {
                    player_x = (float)(nx * CELL_SIZE) + CELL_SIZE * 0.5f;
                    player_y = (float)(ny * CELL_SIZE) + CELL_SIZE * 0.5f;
                    return;
                }
            }
        }
    }
}

void player_update(float dt) {
    if (g.state != STATE_PLAYING) return;

    /* Push out if we somehow landed inside a wall/claimed cell */
    resolve_overlap();

    joypad_inputs_t in = joypad_get_inputs(JOYPAD_PORT_1);
    float sx = in.stick_x;
    float sy = -in.stick_y;   /* Y axis is inverted */

    if (fabsf(sx) < STICK_DEADZONE) sx = 0.f;
    if (fabsf(sy) < STICK_DEADZONE) sy = 0.f;

    sx /= 85.f;
    sy /= 85.f;
    if (sx >  1.f) sx =  1.f;
    if (sx < -1.f) sx = -1.f;
    if (sy >  1.f) sy =  1.f;
    if (sy < -1.f) sy = -1.f;

    float nx = player_x + sx * CURSOR_SPEED * dt;
    float ny = player_y + sy * CURSOR_SPEED * dt;

    /* Clamp to playfield interior */
    if (nx < CELL_SIZE + 1.f)             nx = CELL_SIZE + 1.f;
    if (nx > (GRID_W-2)*CELL_SIZE - 1.f)  nx = (GRID_W-2)*CELL_SIZE - 1.f;
    if (ny < CELL_SIZE + 1.f)             ny = CELL_SIZE + 1.f;
    if (ny > (GRID_H-2)*CELL_SIZE - 1.f)  ny = (GRID_H-2)*CELL_SIZE - 1.f;

    /* Resolve X and Y axes independently so the cursor always slides along walls
       instead of getting pinched. When crossing into a solid cell on one axis,
       snap back to just inside the current cell boundary on that axis. */
    int cur_cx = field_cell_x(player_x);
    int cur_cy = field_cell_y(player_y);

    /* X axis */
    int new_cx = field_cell_x(nx);
    if (new_cx != cur_cx && field_is_solid(new_cx, cur_cy)) {
        /* Snap to the edge of the current cell, leaving 1px gap */
        nx = (nx > player_x)
             ? (float)(cur_cx * CELL_SIZE + CELL_SIZE) - 1.f
             : (float)(cur_cx * CELL_SIZE) + 1.f;
        new_cx = cur_cx;
    }
    player_x = nx;

    /* Y axis (use updated new_cx for diagonal corner cases) */
    int new_cy = field_cell_y(ny);
    if (new_cy != cur_cy && field_is_solid(new_cx, new_cy)) {
        ny = (ny > player_y)
             ? (float)(cur_cy * CELL_SIZE + CELL_SIZE) - 1.f
             : (float)(cur_cy * CELL_SIZE) + 1.f;
    }
    player_y = ny;

    /* A button: place wall */
    joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if (pressed.a) {
        int cx = field_cell_x(player_x);
        int cy = field_cell_y(player_y);
        int horiz = (fabsf(sx) >= fabsf(sy));
        place_wall(cx, cy, horiz);
    }
}
