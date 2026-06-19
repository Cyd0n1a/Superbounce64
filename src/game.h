#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 240

#define CELL_SIZE   4
#define GRID_W      (SCREEN_W / CELL_SIZE)   /* 80 */
#define GRID_H      (SCREEN_H / CELL_SIZE)   /* 60 */

#define MAX_BALLS   8
#define CLAIM_TARGET 75   /* percent to advance */
#define MAX_LIVES   3

typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_WALL_FAIL,
    STATE_LEVEL_COMPLETE,
    STATE_GAME_OVER,
} GameState;

typedef struct {
    GameState state;
    int       level;
    int       lives;
    int       claimed_pct;
    float     state_timer;   /* seconds remaining in timed states */
    int       score;
    int       high_score;
    int       new_high_score;  /* set to 1 briefly when high score is beaten */
} Game;

extern Game g;

void game_init(void);
void game_next_level(void);

#endif
