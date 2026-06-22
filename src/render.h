#ifndef RENDER_H
#define RENDER_H

#include <libdragon.h>

void render_init(void);
void render_frame(surface_t *disp);
void render_set_time(float t);
void render_video_free(void);

#endif
