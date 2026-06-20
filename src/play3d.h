#ifndef PLAY3D_H
#define PLAY3D_H

#include <libdragon.h>

void       play3d_init(void);
void       play3d_destroy(void);
surface_t *play3d_get_zbuf(void);
void       play3d_draw(float time);

#endif
