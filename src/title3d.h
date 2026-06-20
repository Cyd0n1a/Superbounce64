#ifndef TITLE3D_H
#define TITLE3D_H

#include <libdragon.h>

void       title3d_init(void);
void       title3d_destroy(void);
surface_t *title3d_get_zbuf(void);
void       title3d_draw(float time);

#endif
