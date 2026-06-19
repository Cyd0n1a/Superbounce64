#include "game.h"
#include "field.h"
#include "balls.h"
#include "player.h"

Game g;

void game_init(void) {
    g.level       = 1;
    g.lives       = MAX_LIVES;
    g.claimed_pct = 0;
    g.state       = STATE_TITLE;
    g.state_timer = 0.f;
}

void game_next_level(void) {
    field_init();
    field_mark_border();

    int   count = 2 + (g.level - 1);
    if (count > MAX_BALLS) count = MAX_BALLS;
    float speed = 60.f * (1.f + (g.level - 1) * 0.15f);
    balls_init(count, speed);

    player_init();

    g.claimed_pct = field_claimed_percent();
    g.state       = STATE_PLAYING;
    g.state_timer = 0.f;
}
