#include "sfx.h"
#include <libdragon.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE  44100
#define MAX_VOICES   4

static xm64player_t music_player;
static bool         music_loaded;

typedef struct {
    int   active;
    int   pos;
    int   len;
    float phase;
    float freq_start;
    float freq_end;
    int   wave;          /* 0=sine  1=square  2=noise */
    float volume;
    float note_freqs[4];
    int   note_len;
    int   num_notes;
} Voice;

static Voice voices[MAX_VOICES];
static int   bounce_cooldown;

/* ------------------------------------------------------------------ */

static void voice_start(int slot, Voice *cfg) {
    voices[slot] = *cfg;
    voices[slot].active = 1;
    voices[slot].pos    = 0;
    voices[slot].phase  = 0.f;
}

void sfx_play(SfxId id) {
    Voice v;
    memset(&v, 0, sizeof(v));

    switch (id) {
        case SFX_WALL_PLACE:
            v.len        = (int)(SAMPLE_RATE * 0.15f);
            v.freq_start = 900.f;
            v.freq_end   = 150.f;
            v.wave       = 0;
            v.volume     = 0.40f;
            voice_start(0, &v);
            break;

        case SFX_WALL_FAIL:
            v.len        = (int)(SAMPLE_RATE * 0.30f);
            v.freq_start = 90.f;
            v.freq_end   = 90.f;
            v.wave       = 1;
            v.volume     = 0.50f;
            voice_start(1, &v);
            break;

        case SFX_LEVEL_COMPLETE:
            v.len           = (int)(SAMPLE_RATE * 0.36f);
            v.wave          = 0;
            v.volume        = 0.45f;
            v.num_notes     = 3;
            v.note_len      = (int)(SAMPLE_RATE * 0.12f);
            v.note_freqs[0] = 523.25f;
            v.note_freqs[1] = 659.25f;
            v.note_freqs[2] = 783.99f;
            voice_start(2, &v);
            break;

        case SFX_BALL_BOUNCE:
            if (bounce_cooldown > 0) return;
            v.len        = (int)(SAMPLE_RATE * 0.04f);
            v.freq_start = 0.f;
            v.wave       = 2;
            v.volume     = 0.18f;
            voice_start(3, &v);
            bounce_cooldown = (int)(SAMPLE_RATE * 0.08f);
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Add procedural SFX on top of the buffer already filled by mixer_poll */

static void sfx_mix_into(short *buf, size_t n_frames) {
    if (bounce_cooldown > 0)
        bounce_cooldown -= (int)n_frames;

    for (size_t i = 0; i < n_frames; i++) {
        float sample = 0.f;

        for (int v = 0; v < MAX_VOICES; v++) {
            Voice *vp = &voices[v];
            if (!vp->active) continue;

            float t = (float)vp->pos / (float)vp->len;

            float freq;
            if (vp->num_notes > 0) {
                int ni = vp->pos / vp->note_len;
                if (ni >= vp->num_notes) ni = vp->num_notes - 1;
                freq = vp->note_freqs[ni];
            } else {
                freq = vp->freq_start + (vp->freq_end - vp->freq_start) * t;
            }

            vp->phase += freq / (float)SAMPLE_RATE;
            if (vp->phase >= 1.f) vp->phase -= 1.f;

            float osc;
            switch (vp->wave) {
                case 0: osc = sinf(vp->phase * 6.28318f); break;
                case 1: osc = (vp->phase < 0.5f) ? 1.f : -1.f; break;
                default: osc = ((float)rand() / (float)RAND_MAX) * 2.f - 1.f; break;
            }

            float env;
            if (vp->num_notes > 0) {
                float note_t = (float)(vp->pos % vp->note_len) / (float)vp->note_len;
                env = 1.f - note_t * note_t;
            } else {
                env = 1.f - t;
            }

            sample += osc * env * vp->volume;

            vp->pos++;
            if (vp->pos >= vp->len) vp->active = 0;
        }

        if (sample >  1.f) sample =  1.f;
        if (sample < -1.f) sample = -1.f;

        short sfx = (short)(sample * 28000.f);
        int L = (int)buf[i * 2]     + sfx;
        int R = (int)buf[i * 2 + 1] + sfx;
        buf[i * 2]     = (short)(L >  32767 ?  32767 : L < -32768 ? -32768 : L);
        buf[i * 2 + 1] = (short)(R >  32767 ?  32767 : R < -32768 ? -32768 : R);
    }
}

/* ------------------------------------------------------------------ */

void sfx_init(void) {
    memset(voices, 0, sizeof(voices));
    bounce_cooldown = 0;
    music_loaded    = false;

    audio_init(SAMPLE_RATE, 4);
    mixer_init(32);

    int fh = dfs_open("rom:/mozartku.xm64");
    if (fh >= 0) {
        dfs_close(fh);
        xm64player_open(&music_player, "rom:/mozartku.xm64");
        xm64player_set_vol(&music_player, 0.75f);
        xm64player_play(&music_player, 0);
        music_loaded = true;
    }
}

void sfx_poll(void) {
    while (audio_can_write()) {
        short *buf = audio_write_begin();
        int n = audio_get_buffer_length();
        mixer_poll(buf, n);
        sfx_mix_into(buf, (size_t)n);
        audio_write_end();
    }
}
