#include "field.h"
#include <string.h>
#include <stdint.h>

uint8_t field[GRID_H][GRID_W];

/* BFS queue — two parallel arrays for x and y */
static uint8_t qx[GRID_W * GRID_H];
static uint8_t qy[GRID_W * GRID_H];

void field_init(void) {
    memset(field, CELL_EMPTY, sizeof(field));
}

void field_mark_border(void) {
    for (int x = 0; x < GRID_W; x++) {
        field[0][x]          = CELL_WALL;
        field[GRID_H-1][x]   = CELL_WALL;
    }
    for (int y = 0; y < GRID_H; y++) {
        field[y][0]          = CELL_WALL;
        field[y][GRID_W-1]   = CELL_WALL;
    }
}

int field_flood_and_claim(float *ball_xs, float *ball_ys, int num_balls) {
    /* visited array: 0 = not visited, 1 = visited (reachable from a ball) */
    static uint8_t visited[GRID_H][GRID_W];
    memset(visited, 0, sizeof(visited));

    int head = 0, tail = 0;

    /* Seed BFS from each ball's cell */
    for (int i = 0; i < num_balls; i++) {
        int cx = (int)ball_xs[i] / CELL_SIZE;
        int cy = (int)ball_ys[i] / CELL_SIZE;
        if (cx < 0) cx = 0;
        if (cx >= GRID_W) cx = GRID_W - 1;
        if (cy < 0) cy = 0;
        if (cy >= GRID_H) cy = GRID_H - 1;
        if (!visited[cy][cx] && field[cy][cx] == CELL_EMPTY) {
            visited[cy][cx] = 1;
            qx[tail] = (uint8_t)cx;
            qy[tail] = (uint8_t)cy;
            tail++;
        }
    }

    static const int dx[] = { 1, -1, 0,  0 };
    static const int dy[] = { 0,  0, 1, -1 };

    while (head < tail) {
        int cx = qx[head];
        int cy = qy[head];
        head++;
        for (int d = 0; d < 4; d++) {
            int nx = cx + dx[d];
            int ny = cy + dy[d];
            if (nx < 0 || nx >= GRID_W || ny < 0 || ny >= GRID_H) continue;
            if (visited[ny][nx]) continue;
            if (field[ny][nx] != CELL_EMPTY) continue;
            visited[ny][nx] = 1;
            qx[tail] = (uint8_t)nx;
            qy[tail] = (uint8_t)ny;
            tail++;
        }
    }

    /* Any EMPTY cell not visited is now claimed */
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++)
            if (field[y][x] == CELL_EMPTY && !visited[y][x])
                field[y][x] = CELL_CLAIMED;

    return field_claimed_percent();
}

int field_claimed_percent(void) {
    int total = 0, claimed = 0;
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++) {
            if (field[y][x] != CELL_WALL) total++;
            if (field[y][x] == CELL_CLAIMED) claimed++;
        }
    if (total == 0) return 100;
    return claimed * 100 / total;
}
