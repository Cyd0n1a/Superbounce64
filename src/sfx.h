#ifndef SFX_H
#define SFX_H

typedef enum {
    SFX_WALL_PLACE,
    SFX_WALL_FAIL,
    SFX_LEVEL_COMPLETE,
    SFX_BALL_BOUNCE,
} SfxId;

void sfx_init(void);
void sfx_play(SfxId id);
void sfx_poll(void);   /* call once per main loop iteration */

#endif
