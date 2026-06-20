#include "title3d.h"
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <libdragon.h>
#include <math.h>

/* UV-sphere mesh parameters */
#define NLAT         8    /* latitude bands          */
#define NLON        16    /* longitude quads per band */
#define SPHERE_R    28.f  /* object-space radius      */

/* Two T3DVertPacked structs (= 4 verts) per quad, NLON quads per strip */
#define STRIP_PACKED  (NLON * 2)
#define TOTAL_PACKED  (NLAT * STRIP_PACKED)

static T3DVertPacked *sphere_verts;  /* uncached single buffer, safe via rspq_wait */
static T3DMat4FP     *model_mat_fp;
static T3DViewport    sphere_vp;
static surface_t      zbuf;          /* 16-bit depth surface for the 3D sphere */

/* ------------------------------------------------------------------ */

static uint32_t hue_rgba(float h) {
    h -= (float)(int)h;
    if (h < 0.f) h += 1.f;
    float s = h * 6.f;
    int   i = (int)s;
    float f = s - (float)i;
    float r, g, b;
    switch (i % 6) {
        case 0: r=1.f;   g=f;    b=0.f;  break;
        case 1: r=1.f-f; g=1.f;  b=0.f;  break;
        case 2: r=0.f;   g=1.f;  b=f;    break;
        case 3: r=0.f;   g=1.f-f;b=1.f;  break;
        case 4: r=f;     g=0.f;  b=1.f;  break;
        default:r=1.f;   g=0.f;  b=1.f-f;break;
    }
    return ((uint8_t)(r * 255.f) << 24)
         | ((uint8_t)(g * 255.f) << 16)
         | ((uint8_t)(b * 255.f) <<  8)
         | 0xFF;
}

/* ------------------------------------------------------------------ */

void title3d_init(void) {
    sphere_verts = malloc_uncached(sizeof(T3DVertPacked) * TOTAL_PACKED);
    model_mat_fp = malloc_uncached(sizeof(T3DMat4FP));
    /* Triple-buffer the viewport matrices (3 display buffers) */
    sphere_vp    = t3d_viewport_create_buffered(3);
    /* Allocate a single depth buffer — shared across display buffers,
     * which is safe because rspq_wait() in render_frame ensures the RDP
     * finishes writing it before the next frame reads/overwrites it.    */
    zbuf = surface_alloc(FMT_RGBA16, 320, 240);
}

surface_t *title3d_get_zbuf(void) { return &zbuf; }

void title3d_destroy(void) {
    surface_free(&zbuf);
    t3d_viewport_destroy(&sphere_vp);
    free_uncached(model_mat_fp);
    free_uncached(sphere_verts);
}

/* ------------------------------------------------------------------ */

void title3d_draw(float time) {
    /* --- Model matrix: spin on Y, nod on X, squish scale --- */
    float sq       = sinf(time * 2.3f) * 0.28f;
    float scale[3] = { 1.f + sq, 1.f - sq * 1.3f, 1.f + sq };
    float rot[3]   = {
        sinf(time * 0.71f) * 0.28f,   /* nod    */
        time * 0.95f,                   /* spin   */
        sinf(time * 0.53f) * 0.12f,   /* tilt   */
    };
    float trans[3] = { 0, 0, 0 };
    t3d_mat4fp_from_srt_euler(model_mat_fp, scale, rot, trans);

    /* --- Camera --- */
    T3DVec3 cam_pos    = {{0, 5.f, -80.f}};
    T3DVec3 cam_target = {{0, 0,   0}};
    T3DVec3 up         = {{0, 1,   0}};
    t3d_viewport_set_projection(&sphere_vp, T3D_DEG_TO_RAD(70.f), 5.f, 200.f);
    t3d_viewport_look_at(&sphere_vp, &cam_pos, &cam_target, &up);

    /* --- Build vertex buffer (colours animate, positions static) ---
     * Safe to write to sphere_verts: render_frame calls rspq_wait() before
     * returning, so the RSP has finished reading the previous frame's data
     * before this frame writes new colours.                               */
    for (int i = 0; i < NLAT; i++) {
        float lat0 = (float) i      / NLAT;
        float lat1 = (float)(i + 1) / NLAT;
        float th0  = T3D_PI * lat0;
        float th1  = T3D_PI * lat1;
        float sin0 = sinf(th0), cos0 = cosf(th0);
        float sin1 = sinf(th1), cos1 = cosf(th1);

        for (int j = 0; j < NLON; j++) {
            float lon0 = (float) j      / NLON;
            float lon1 = (float)(j + 1) / NLON;
            float ph0  = 2.f * T3D_PI * lon0;
            float ph1  = 2.f * T3D_PI * lon1;
            float cp0  = cosf(ph0), sp0 = sinf(ph0);
            float cp1  = cosf(ph1), sp1 = sinf(ph1);

            /* Four corners of this quad on the unit sphere × SPHERE_R */
            int16_t pA[3] = { (int16_t)(sin0*cp0*SPHERE_R), (int16_t)(cos0*SPHERE_R), (int16_t)(sin0*sp0*SPHERE_R) };
            int16_t pB[3] = { (int16_t)(sin0*cp1*SPHERE_R), (int16_t)(cos0*SPHERE_R), (int16_t)(sin0*sp1*SPHERE_R) };
            int16_t pC[3] = { (int16_t)(sin1*cp0*SPHERE_R), (int16_t)(cos1*SPHERE_R), (int16_t)(sin1*sp0*SPHERE_R) };
            int16_t pD[3] = { (int16_t)(sin1*cp1*SPHERE_R), (int16_t)(cos1*SPHERE_R), (int16_t)(sin1*sp1*SPHERE_R) };

            /* Rainbow hue: stripes that scroll over time */
            float base  = time * 0.25f;
            uint32_t cA = hue_rgba(lat0 * 0.5f + lon0 + base);
            uint32_t cB = hue_rgba(lat0 * 0.5f + lon1 + base);
            uint32_t cC = hue_rgba(lat1 * 0.5f + lon0 + base);
            uint32_t cD = hue_rgba(lat1 * 0.5f + lon1 + base);

            /* Quad j uses cache slots j*4 .. j*4+3.
             * Pack: slot 0=pA, 1=pB, 2=pC, 3=pD so that
             * t3d_quad_draw_unindexed produces [0,1,2]+[3,2,1] correctly. */
            T3DVertPacked *vp = &sphere_verts[i * STRIP_PACKED + j * 2];
            vp[0] = (T3DVertPacked){
                .posA = {pA[0], pA[1], pA[2]}, .normA = 0, .rgbaA = cA, .stA = {0, 0},
                .posB = {pB[0], pB[1], pB[2]}, .normB = 0, .rgbaB = cB, .stB = {0, 0},
            };
            vp[1] = (T3DVertPacked){
                .posA = {pC[0], pC[1], pC[2]}, .normA = 0, .rgbaA = cC, .stA = {0, 0},
                .posB = {pD[0], pD[1], pD[2]}, .normB = 0, .rgbaB = cD, .stB = {0, 0},
            };
        }
    }

    /* --- RSP/RDP draw pass --- */
    t3d_frame_start();          /* reset RDP mode; enables AA, zbuf, persp  */
    t3d_screen_clear_depth();   /* clear zbuf only (colour already drawn)   */
    t3d_viewport_attach(&sphere_vp);

    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    rdpq_mode_zbuf(true, true);

    t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH);
    t3d_light_set_count(0);
    t3d_light_set_ambient((uint8_t[]){0xFF, 0xFF, 0xFF, 0xFF});

    t3d_matrix_push(model_mat_fp);
    for (int i = 0; i < NLAT; i++) {
        /* Load one strip (NLON*4 individual verts = NLON*2 T3DVertPacked) */
        t3d_vert_load(sphere_verts + i * STRIP_PACKED, 0, NLON * 4);
        t3d_quad_draw_unindexed(0, NLON);
        t3d_tri_sync();
    }
    t3d_matrix_pop(1);
}
