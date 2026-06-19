#include <libdragon.h>
#include "game.h"
#include "field.h"
#include "balls.h"
#include "player.h"
#include "effects.h"
#include "render.h"
#include "sfx.h"

#define STEP_US 16667   /* ~60 Hz fixed timestep */

static void update(float dt) {
    if (g.state == STATE_TITLE) {
        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        if (pressed.start || pressed.a) {
            game_next_level();
        }
        return;
    }

    if (g.state == STATE_WALL_FAIL || g.state == STATE_LEVEL_COMPLETE) {
        g.state_timer -= dt;
        if (g.state_timer <= 0.f) {
            if (g.state == STATE_WALL_FAIL) {
                g.lives--;
                if (g.lives <= 0)
                    g.state = STATE_GAME_OVER;
                else
                    g.state = STATE_PLAYING;
            } else {
                g.level++;
                game_next_level();
            }
        }
        balls_update(dt);
        return;
    }

    if (g.state == STATE_GAME_OVER) {
        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        if (pressed.start) game_init();
        return;
    }

    balls_update(dt);
    player_update(dt);
}

int main(void) {
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    joypad_init();
    timer_init();

    sfx_init();
    effects_init();
    game_init();

    long long prev  = timer_ticks();
    long long accum = 0;

    while (1) {
        long long now  = timer_ticks();
        long long diff = TIMER_MICROS_LL(now - prev);
        prev = now;

        /* Cap to avoid spiral of death after a stall */
        if (diff > 100000) diff = 100000;
        accum += diff;

        while (accum >= STEP_US) {
            joypad_poll();
            update(STEP_US / 1000000.f);
            accum -= STEP_US;
        }

        /* Effects are purely visual — update once per frame with real elapsed time */
        effects_update((float)diff / 1000000.f);

        sfx_poll();

        static float total_time = 0.f;
        total_time += (float)diff / 1000000.f;
        render_set_time(total_time);

        surface_t *disp = display_get();
        render_frame(disp);
    }

    return 0;
}
