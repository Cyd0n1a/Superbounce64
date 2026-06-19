#ifndef EFFECTS_H
#define EFFECTS_H

#include <libdragon.h>

void effects_init(void);
void effects_update(float dt);
void effects_draw_background(surface_t *surf);

/* Flash overlay for wall fail / level complete */
void effects_draw_flash(float r, float g, float b, float alpha);

#endif
