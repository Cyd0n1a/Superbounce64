#ifndef PLAYER_H
#define PLAYER_H

void player_init(void);
void player_update(float dt);
void player_rumble_tick(void);

extern float player_x, player_y;

/* Wall preview — updated every frame, consumed by render.c */
typedef enum {
    PREVIEW_NONE = 0,   /* stick in deadzone */
    PREVIEW_CLEAR,      /* wall would succeed */
    PREVIEW_BLOCKED,    /* ball in path */
} PreviewState;

extern PreviewState preview_state;
extern float preview_x0, preview_y0;   /* screen-space endpoints */
extern float preview_x1, preview_y1;

#endif
