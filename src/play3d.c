#include "play3d.h"
#include "balls.h"
#include "field.h"
#include "game.h"
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <libdragon.h>
#include <math.h>

/* ------------------------------------------------------------------ */
/* Constants                                                           */
/* ------------------------------------------------------------------ */

/* Low-poly sphere for gameplay balls */
#define BNLAT        4
#define BNLON        8
#define BSPHERE_R    6.5f
#define BSTRIP_PACKED  (BNLON * 2)
#define BBALL_PACKED   (BNLAT * BSTRIP_PACKED)
#define BMAX_BALLS     8

/* Camera at (0, 0, -120) with 90° FOV → 1 world unit = 1 pixel at z=0 */
#define CAM_Z   -120.f
#define SCR_CX   160.f
#define SCR_CY   120.f

/* Wall segment budget — border + placed walls never exceed this */
#define MAX_WALL_SEGS  256

/* ------------------------------------------------------------------ */
/* State                                                               */
/* ------------------------------------------------------------------ */

static T3DVertPacked *ball_verts;  /* BMAX_BALLS * BBALL_PACKED */
static T3DMat4FP     *ball_mats;   /* BMAX_BALLS */
static T3DMat4FP     *wall_imat;   /* uncached identity model matrix for wall passes */
static T3DVertPacked *wall_verts;  /* MAX_WALL_SEGS * 2, reused per glow pass */
static T3DViewport    play_vp;

/* Neon colours per ball index — matches BALL_COLORS in render.c */
static const uint8_t BCOLORS[8][3] = {
    {   0, 255, 255 },
    { 255,   0, 255 },
    { 255, 255,   0 },
    {   0, 255,  51 },
    { 255, 128,   0 },
    { 255, 255, 255 },
    { 255,  51, 128 },
    {  51, 204, 255 },
};

/* ------------------------------------------------------------------ */
/* Init / destroy                                                      */
/* ------------------------------------------------------------------ */

void play3d_init(void) {
    ball_verts = malloc_uncached(sizeof(T3DVertPacked) * BMAX_BALLS * BBALL_PACKED);
    ball_mats  = malloc_uncached(sizeof(T3DMat4FP)     * BMAX_BALLS);
    wall_imat  = malloc_uncached(sizeof(T3DMat4FP));
    wall_verts = malloc_uncached(sizeof(T3DVertPacked) * MAX_WALL_SEGS * 2);
    play_vp    = t3d_viewport_create_buffered(3);
    t3d_mat4fp_identity(wall_imat);
}

void play3d_destroy(void) {
    t3d_viewport_destroy(&play_vp);
    free_uncached(wall_verts);
    free_uncached(wall_imat);
    free_uncached(ball_mats);
    free_uncached(ball_verts);
}

/* ------------------------------------------------------------------ */
/* Ball sphere vertices                                                */
/* ------------------------------------------------------------------ */

static void build_ball_verts(int bi) {
    T3DVertPacked *verts = ball_verts + bi * BBALL_PACKED;
    const uint8_t *col   = BCOLORS[balls[bi].color_idx];
    uint32_t rgba = ((uint32_t)col[0]<<24)|((uint32_t)col[1]<<16)|((uint32_t)col[2]<<8)|0xFF;

    for (int i = 0; i < BNLAT; i++) {
        float th0 = T3D_PI * (float)i       / BNLAT;
        float th1 = T3D_PI * (float)(i + 1) / BNLAT;
        float sin0 = sinf(th0), cos0 = cosf(th0);
        float sin1 = sinf(th1), cos1 = cosf(th1);

        for (int j = 0; j < BNLON; j++) {
            float ph0 = 2.f * T3D_PI * (float)j       / BNLON;
            float ph1 = 2.f * T3D_PI * (float)(j + 1) / BNLON;
            float cp0 = cosf(ph0), sp0 = sinf(ph0);
            float cp1 = cosf(ph1), sp1 = sinf(ph1);

            /* Unit normals double as normalised positions for a unit sphere */
            float nAx=sin0*cp0, nAy=cos0, nAz=sin0*sp0;
            float nBx=sin0*cp1, nBy=cos0, nBz=sin0*sp1;
            float nCx=sin1*cp0, nCy=cos1, nCz=sin1*sp0;
            float nDx=sin1*cp1, nDy=cos1, nDz=sin1*sp1;

            T3DVec3 nA = {{nAx,nAy,nAz}}, nB = {{nBx,nBy,nBz}};
            T3DVec3 nC = {{nCx,nCy,nCz}}, nD = {{nDx,nDy,nDz}};

            T3DVertPacked *vp = verts + i * BSTRIP_PACKED + j * 2;
            vp[0] = (T3DVertPacked){
                .posA={(int16_t)(nAx*BSPHERE_R),(int16_t)(nAy*BSPHERE_R),(int16_t)(nAz*BSPHERE_R)},
                .normA=t3d_vert_pack_normal(&nA), .rgbaA=rgba, .stA={0,0},
                .posB={(int16_t)(nBx*BSPHERE_R),(int16_t)(nBy*BSPHERE_R),(int16_t)(nBz*BSPHERE_R)},
                .normB=t3d_vert_pack_normal(&nB), .rgbaB=rgba, .stB={0,0},
            };
            vp[1] = (T3DVertPacked){
                .posA={(int16_t)(nCx*BSPHERE_R),(int16_t)(nCy*BSPHERE_R),(int16_t)(nCz*BSPHERE_R)},
                .normA=t3d_vert_pack_normal(&nC), .rgbaA=rgba, .stA={0,0},
                .posB={(int16_t)(nDx*BSPHERE_R),(int16_t)(nDy*BSPHERE_R),(int16_t)(nDz*BSPHERE_R)},
                .normB=t3d_vert_pack_normal(&nD), .rgbaB=rgba, .stB={0,0},
            };
        }
    }
}

/* ------------------------------------------------------------------ */
/* Wall flat quads                                                     */
/* ------------------------------------------------------------------ */

typedef struct { float x0, y0, x1, y1; int border; } WallSeg;
static WallSeg wall_segs[MAX_WALL_SEGS];
static int     num_wall_segs;

static void collect_wall_segs(void) {
    num_wall_segs = 0;

    for (int cy = 0; cy < GRID_H && num_wall_segs < MAX_WALL_SEGS - 1; cy++) {
        int border_row = (cy == 0 || cy == GRID_H - 1);
        int run_start = -1;
        for (int cx = 0; cx <= GRID_W; cx++) {
            int is_wall = (cx < GRID_W) && (field[cy][cx] == CELL_WALL);
            if (is_wall && run_start < 0) {
                run_start = cx;
            } else if (!is_wall && run_start >= 0) {
                float y = cy * CELL_SIZE + CELL_SIZE * 0.5f;
                wall_segs[num_wall_segs++] = (WallSeg){
                    run_start * CELL_SIZE, y, cx * CELL_SIZE, y, border_row
                };
                run_start = -1;
            }
        }
    }

    for (int cx = 0; cx < GRID_W && num_wall_segs < MAX_WALL_SEGS - 1; cx++) {
        int border_col = (cx == 0 || cx == GRID_W - 1);
        int run_start = -1;
        for (int cy = 0; cy <= GRID_H; cy++) {
            int is_wall = (cy < GRID_H) && (field[cy][cx] == CELL_WALL);
            if (is_wall && run_start < 0) {
                run_start = cy;
            } else if (!is_wall && run_start >= 0) {
                float x = cx * CELL_SIZE + CELL_SIZE * 0.5f;
                wall_segs[num_wall_segs++] = (WallSeg){
                    x, run_start * CELL_SIZE, x, cy * CELL_SIZE, border_col
                };
                run_start = -1;
            }
        }
    }
}

/* Build T3DVertPacked quads for segments matching border flag; returns count */
static int build_wall_pass(float half_t, uint32_t rgba, int want_border) {
    int count = 0;
    for (int k = 0; k < num_wall_segs && count < MAX_WALL_SEGS; k++) {
        if (wall_segs[k].border != want_border) continue;

        /* Convert screen coords to world coords */
        float wx0 = wall_segs[k].x0 - SCR_CX,  wy0 = SCR_CY - wall_segs[k].y0;
        float wx1 = wall_segs[k].x1 - SCR_CX,  wy1 = SCR_CY - wall_segs[k].y1;

        float dx = wx1 - wx0, dy = wy1 - wy0;
        float len = sqrtf(dx*dx + dy*dy);
        if (len < 0.1f) continue;
        float nx = -dy / len * half_t;
        float ny =  dx / len * half_t;

        T3DVertPacked *vp = &wall_verts[count * 2];
        vp[0] = (T3DVertPacked){
            .posA={(int16_t)(wx0+nx),(int16_t)(wy0+ny),0},.normA=0,.rgbaA=rgba,.stA={0,0},
            .posB={(int16_t)(wx0-nx),(int16_t)(wy0-ny),0},.normB=0,.rgbaB=rgba,.stB={0,0},
        };
        vp[1] = (T3DVertPacked){
            .posA={(int16_t)(wx1+nx),(int16_t)(wy1+ny),0},.normA=0,.rgbaA=rgba,.stA={0,0},
            .posB={(int16_t)(wx1-nx),(int16_t)(wy1-ny),0},.normB=0,.rgbaB=rgba,.stB={0,0},
        };
        count++;
    }
    return count;
}

/* Batch-submit wall quads to the RSP (vertex cache holds 17 quads max) */
static void draw_wall_batch(int count) {
    int offset = 0;
    while (offset < count) {
        int batch = (count - offset > 17) ? 17 : (count - offset);
        t3d_vert_load(wall_verts + offset * 2, 0, batch * 4);
        t3d_quad_draw_unindexed(0, batch);
        t3d_tri_sync();
        offset += batch;
    }
}

/* ------------------------------------------------------------------ */
/* Main draw entry point                                               */
/* ------------------------------------------------------------------ */

void play3d_draw(float time) {
    /* Camera: maps game pixels 1:1 to world units at z=0.
     * Game (bx, by) → world (bx-160, 120-by, 0).             */
    T3DVec3 cam_pos    = {{ 0,  0, CAM_Z }};
    T3DVec3 cam_target = {{ 0,  0, 0 }};
    T3DVec3 up         = {{ 0,  1, 0 }};
    t3d_viewport_set_projection(&play_vp, T3D_DEG_TO_RAD(90.f), 1.f, 500.f);
    t3d_viewport_look_at(&play_vp, &cam_pos, &cam_target, &up);

    /* Build ball vertex buffers and model matrices */
    for (int i = 0; i < num_balls; i++) {
        build_ball_verts(i);
        float spd  = sqrtf(balls[i].vx*balls[i].vx + balls[i].vy*balls[i].vy);
        float spin = time * spd * 0.035f + (float)i * 1.1f;
        float scale[3] = { 1.f, 1.f, 1.f };
        float rot[3]   = { sinf(spin * 0.7f) * 0.3f, spin, 0.f };
        float trans[3] = { balls[i].x - SCR_CX, SCR_CY - balls[i].y, -2.f };
        t3d_mat4fp_from_srt_euler(&ball_mats[i], scale, rot, trans);
    }

    /* ---- RSP/RDP draw pass ---- */
    t3d_frame_start();
    t3d_screen_clear_depth();
    t3d_viewport_attach(&play_vp);

    /* === Walls: flat quads in world space, no depth test === */
    collect_wall_segs();

    t3d_state_set_drawflags(T3D_FLAG_SHADED);
    t3d_light_set_count(0);
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_zbuf(false, false);

    /* Border: single dim-blue pass */
    {
        uint32_t rgba = (25u<<24)|(38u<<16)|(89u<<8)|229u;
        int cnt = build_wall_pass(1.f, rgba, 1);
        if (cnt) {
            t3d_matrix_push(wall_imat);
            draw_wall_batch(cnt);
            t3d_matrix_pop(1);
        }
    }

    /* Placed walls: 3-pass glow (thick outer, medium, thin bright core) */
    {
        float pulse = 0.12f + 0.06f * sinf(time * 4.f);
        /* Cyan: 0.4→102, 0.8→204, 1.0→255 */
        static const float HALFT[3] = { 3.5f, 1.5f, 0.5f };

        for (int p = 0; p < 3; p++) {
            float a = (p == 0) ? pulse : (p == 1 ? 120.f/255.f : 1.f);
            uint32_t rgba = (102u<<24)|(204u<<16)|(255u<<8)|((uint8_t)(a*255.f));
            int cnt = build_wall_pass(HALFT[p], rgba, 0);
            if (cnt) {
                t3d_matrix_push(wall_imat);
                draw_wall_batch(cnt);
                t3d_matrix_pop(1);
            }
        }
    }

    /* === Balls: lit 3D spheres with depth test === */
    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH);
    t3d_light_set_count(1);
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    rdpq_mode_zbuf(true, true);

    T3DVec3 light_dir = {{ 0.577f, 0.577f, -0.577f }};

    for (int i = 0; i < num_balls; i++) {
        const uint8_t *col = BCOLORS[balls[i].color_idx];
        uint8_t amb[4]    = { col[0]/4, col[1]/4, col[2]/4, 255 };
        uint8_t bright[4] = { col[0],   col[1],   col[2],   255 };
        t3d_light_set_ambient(amb);
        t3d_light_set_directional(0, bright, &light_dir);

        t3d_matrix_push(&ball_mats[i]);
        for (int s = 0; s < BNLAT; s++) {
            t3d_vert_load(ball_verts + i * BBALL_PACKED + s * BSTRIP_PACKED, 0, BNLON * 4);
            t3d_quad_draw_unindexed(0, BNLON);
            t3d_tri_sync();
        }
        t3d_matrix_pop(1);
    }
}
