#ifndef SFX_H
#define SFX_H

typedef enum {
    SFX_WALL_PLACE,
    SFX_WALL_FAIL,
    SFX_LEVEL_COMPLETE,
    SFX_BALL_BOUNCE,
    SFX_SPLASH_INTRO,
} SfxId;

void sfx_init(void);
void sfx_start_music(void);
void sfx_play(SfxId id);
void sfx_poll(void);   /* call once per main loop iteration */
void sfx_video_start(void);
void sfx_video_stop(void);

#endif
