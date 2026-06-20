#include "bg3d.h"
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <libdragon.h>
#include <math.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Constants                                                           */
/* ------------------------------------------------------------------ */

#define TUN_RINGS  18    /* ring count: 17 quads per face              */
#define TUN_FACES   4    /* top / right / bottom / left                */
#define TUN_QUADS  (TUN_RINGS - 1)
#define TUN_PACKED (TUN_FACES * TUN_QUADS * 2)  /* 2 T3DVertPacked/quad */
#define TUN_W      160   /* half-width (matches game screen→world map)  */
#define TUN_H      120   /* half-height                                 */

/* CA update rate: every N frames (~60/N Hz) */
#define CA_PERIOD  2

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

static T3DVertPacked *tun_verts;
static T3DMat4FP     *tun_imat;

/* Cellular automaton: [face][ring] hue 0-255 */
static uint8_t ca[TUN_FACES][TUN_RINGS];
static uint8_t ca_tmp[TUN_FACES][TUN_RINGS];
static int     ca_frame;
static int     ca_global_hue;

/* Ring Z coordinates (computed once at init) */
static int16_t ring_z[TUN_RINGS];

/* ------------------------------------------------------------------ */
/* Color                                                               */
/* ------------------------------------------------------------------ */

static uint32_t hue_rgba(uint8_t h, float val) {
    float hf = (float)h * (6.f / 256.f);
    int   i  = (int)hf;
    float f  = hf - (float)i;
    float p  = 0.f;
    float q  = val * (1.f - f);
    float t  = val * f;
    float r, g, b;
    switch (i % 6) {
        case 0: r=val; g=t;   b=p;   break;
        case 1: r=q;   g=val; b=p;   break;
        case 2: r=p;   g=val; b=t;   break;
        case 3: r=p;   g=q;   b=val; break;
        case 4: r=t;   g=p;   b=val; break;
        default:r=val; g=p;   b=q;   break;
    }
    return ((uint8_t)(r*255.f)<<24)
         | ((uint8_t)(g*255.f)<<16)
         | ((uint8_t)(b*255.f)<< 8)
         | 0xFF;
}

/* ------------------------------------------------------------------ */
/* Init / destroy                                                      */
/* ------------------------------------------------------------------ */

void bg3d_init(void) {
    tun_verts = malloc_uncached(sizeof(T3DVertPacked) * TUN_PACKED);
    tun_imat  = malloc_uncached(sizeof(T3DMat4FP));
    t3d_mat4fp_identity(tun_imat);

    /* Exponential ring spacing so rings appear evenly spaced on screen.
     * Camera at z=-120, so distance from camera = z + 120.
     * We map distances [120, 500] exponentially across TUN_RINGS. */
    for (int i = 0; i < TUN_RINGS; i++) {
        float t = (float)i / (float)(TUN_RINGS - 1);
        float d = 120.f * powf(500.f / 120.f, t);
        ring_z[i] = (int16_t)(d - 120.f);
    }

    /* Seed CA with a rainbow phase gradient */
    for (int f = 0; f < TUN_FACES; f++)
        for (int r = 0; r < TUN_RINGS; r++)
            ca[f][r] = (uint8_t)((f * 64 + r * 15) & 0xFF);
}

void bg3d_destroy(void) {
    free_uncached(tun_imat);
    free_uncached(tun_verts);
}

/* ------------------------------------------------------------------ */
/* Cellular automaton                                                  */
/* ------------------------------------------------------------------ */

/* Rule:
 *  - Far end (ring TUN_RINGS-1): inject rotating rainbow, offset 90° per face.
 *  - All other rings: color flows from the ring behind (back→front),
 *    with slight bleed from the two adjacent faces (left/right wrap-around).
 *  - Result: rainbow waves flow from vanishing point toward the viewer,
 *    with slow colour blending around the tunnel corners.              */
static void ca_step(void) {
    ca_global_hue = (ca_global_hue + 3) & 0xFF;

    /* Inject new hue at the far (vanishing) end */
    for (int f = 0; f < TUN_FACES; f++)
        ca[f][TUN_RINGS - 1] = (uint8_t)((ca_global_hue + f * 64) & 0xFF);

    /* Propagate toward the near end */
    for (int r = TUN_RINGS - 2; r >= 0; r--) {
        for (int f = 0; f < TUN_FACES; f++) {
            int self = ca[f][r];
            int back = ca[f][r + 1];                /* ring behind (farther)  */
            int fl   = ca[(f + 3) & 3][r];          /* left face neighbour    */
            int fr   = ca[(f + 1) & 3][r];          /* right face neighbour   */
            /* Weight: 4 parts back-ring, 2 parts self, 1 part each adjacent face */
            int target = (back * 4 + self * 2 + fl + fr) >> 3;
            ca_tmp[f][r] = (uint8_t)((self + target) >> 1);
        }
    }
    for (int f = 0; f < TUN_FACES; f++)
        for (int r = 0; r < TUN_RINGS - 1; r++)
            ca[f][r] = ca_tmp[f][r];
}

/* ------------------------------------------------------------------ */
/* Vertex buffer                                                       */
/* ------------------------------------------------------------------ */

/* Face layout (viewed from camera looking toward +Z):
 *   f=0 TOP:    ceiling, y=+TUN_H, left→right
 *   f=1 RIGHT:  right wall, x=+TUN_W, top→bottom
 *   f=2 BOTTOM: floor, y=-TUN_H, right→left
 *   f=3 LEFT:   left wall, x=-TUN_W, bottom→top
 *
 * For each face f, for each ring segment r→r+1:
 *   slot 0 = A (near-left), slot 1 = B (near-right)
 *   slot 2 = C (far-right), slot 3 = D (far-left)
 * t3d_quad_draw_unindexed draws [A,B,C]+[D,C,B].               */
static void build_tun_verts(void) {
    for (int f = 0; f < TUN_FACES; f++) {
        T3DVertPacked *base = tun_verts + f * TUN_QUADS * 2;

        for (int r = 0; r < TUN_QUADS; r++) {
            /* Brightness fades with depth (0.5 near → 0.2 far) */
            float val_n = 0.5f - (float) r      / TUN_RINGS * 0.3f;
            float val_f = 0.5f - (float)(r + 1) / TUN_RINGS * 0.3f;

            uint32_t col_n = hue_rgba(ca[f][r],     val_n);
            uint32_t col_f = hue_rgba(ca[f][r + 1], val_f);

            int16_t z0 = ring_z[r];
            int16_t z1 = ring_z[r + 1];

            int16_t Ax, Ay, Az, Bx, By, Bz, Cx, Cy, Cz, Dx, Dy, Dz;
            switch (f) {
                case 0: /* TOP */
                    Ax=-TUN_W; Ay= TUN_H; Az=z0;
                    Bx= TUN_W; By= TUN_H; Bz=z0;
                    Cx= TUN_W; Cy= TUN_H; Cz=z1;
                    Dx=-TUN_W; Dy= TUN_H; Dz=z1;
                    break;
                case 1: /* RIGHT */
                    Ax= TUN_W; Ay= TUN_H; Az=z0;
                    Bx= TUN_W; By=-TUN_H; Bz=z0;
                    Cx= TUN_W; Cy=-TUN_H; Cz=z1;
                    Dx= TUN_W; Dy= TUN_H; Dz=z1;
                    break;
                case 2: /* BOTTOM */
                    Ax= TUN_W; Ay=-TUN_H; Az=z0;
                    Bx=-TUN_W; By=-TUN_H; Bz=z0;
                    Cx=-TUN_W; Cy=-TUN_H; Cz=z1;
                    Dx= TUN_W; Dy=-TUN_H; Dz=z1;
                    break;
                default: /* LEFT */
                    Ax=-TUN_W; Ay=-TUN_H; Az=z0;
                    Bx=-TUN_W; By= TUN_H; Bz=z0;
                    Cx=-TUN_W; Cy= TUN_H; Cz=z1;
                    Dx=-TUN_W; Dy=-TUN_H; Dz=z1;
                    break;
            }

            base[r * 2 + 0] = (T3DVertPacked){
                .posA={Ax,Ay,Az},.normA=0,.rgbaA=col_n,.stA={0,0},
                .posB={Bx,By,Bz},.normB=0,.rgbaB=col_n,.stB={0,0},
            };
            base[r * 2 + 1] = (T3DVertPacked){
                .posA={Cx,Cy,Cz},.normA=0,.rgbaA=col_f,.stA={0,0},
                .posB={Dx,Dy,Dz},.normB=0,.rgbaB=col_f,.stB={0,0},
            };
        }
    }
}

/* ------------------------------------------------------------------ */
/* Draw                                                                */
/* ------------------------------------------------------------------ */

/* Called from inside an active t3d frame (after t3d_frame_start +
 * t3d_viewport_attach), draws the tunnel in the current viewport.    */
void bg3d_draw(void) {
    if (++ca_frame >= CA_PERIOD) {
        ca_frame = 0;
        ca_step();
    }

    build_tun_verts();

    t3d_state_set_drawflags(T3D_FLAG_SHADED);
    t3d_light_set_count(0);
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_zbuf(false, false);

    t3d_matrix_push(tun_imat);
    for (int f = 0; f < TUN_FACES; f++) {
        /* All TUN_QUADS quads for one face fit in one RSP load (68 verts < 70) */
        t3d_vert_load(tun_verts + f * TUN_QUADS * 2, 0, TUN_QUADS * 4);
        t3d_quad_draw_unindexed(0, TUN_QUADS);
        t3d_tri_sync();
    }
    t3d_matrix_pop(1);
}
