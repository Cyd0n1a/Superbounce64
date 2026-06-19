#include "sfx.h"
#include <libdragon.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE  22050
#define MAX_VOICES   4

/* --- Ambient music --- */
/* Three sine drones forming an Am chord (110, 130.8, 165 Hz) with
   independent slow LFOs that give a gentle pitch drift. */
#define MUSIC_VOICES 3

typedef struct {
    float base_freq;
    float lfo_rate;   /* Hz: how fast the pitch drifts */
    float lfo_depth;  /* Hz: how far the pitch drifts */
    float lfo_phase;  /* 0..1 */
    float osc_phase;  /* 0..1 */
    float volume;
} MusicVoice;

static MusicVoice music_voices[MUSIC_VOICES] = {
    { 110.0f, 0.13f, 2.5f, 0.0f, 0.0f, 0.09f },   /* A2 */
    { 130.8f, 0.08f, 2.0f, 0.3f, 0.0f, 0.07f },   /* C3 */
    { 165.0f, 0.19f, 3.0f, 0.7f, 0.0f, 0.08f },   /* E3 */
};

typedef struct {
    int   active;
    int   pos;           /* samples elapsed */
    int   len;           /* total samples */
    float phase;         /* oscillator phase 0..1 */
    float freq_start;
    float freq_end;
    int   wave;          /* 0=sine  1=square  2=noise */
    float volume;
    /* Optional note sequence (for arpeggio SFX) */
    float note_freqs[4];
    int   note_len;      /* samples per note */
    int   num_notes;
} Voice;

static Voice voices[MAX_VOICES];

/* Rate-limit bounce SFX so rapid bounces don't flood the channel */
static int bounce_cooldown;

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
            /* Descending sine sweep: 900 → 150 Hz over 0.15 s */
            v.len        = (int)(SAMPLE_RATE * 0.15f);
            v.freq_start = 900.f;
            v.freq_end   = 150.f;
            v.wave       = 0;
            v.volume     = 0.40f;
            voice_start(0, &v);
            break;

        case SFX_WALL_FAIL:
            /* Harsh square buzz at 90 Hz, 0.3 s */
            v.len        = (int)(SAMPLE_RATE * 0.30f);
            v.freq_start = 90.f;
            v.freq_end   = 90.f;
            v.wave       = 1;
            v.volume     = 0.50f;
            voice_start(1, &v);
            break;

        case SFX_LEVEL_COMPLETE:
            /* Ascending sine arpeggio: C5 E5 G5, 0.12 s each */
            v.len          = (int)(SAMPLE_RATE * 0.36f);
            v.wave         = 0;
            v.volume       = 0.45f;
            v.num_notes    = 3;
            v.note_len     = (int)(SAMPLE_RATE * 0.12f);
            v.note_freqs[0] = 523.25f;   /* C5 */
            v.note_freqs[1] = 659.25f;   /* E5 */
            v.note_freqs[2] = 783.99f;   /* G5 */
            voice_start(2, &v);
            break;

        case SFX_BALL_BOUNCE:
            if (bounce_cooldown > 0) return;
            v.len        = (int)(SAMPLE_RATE * 0.04f);
            v.freq_start = 0.f;
            v.wave       = 2;   /* noise */
            v.volume     = 0.18f;
            voice_start(3, &v);
            bounce_cooldown = (int)(SAMPLE_RATE * 0.08f);
            break;
    }
}

/* ------------------------------------------------------------------ */

static void sfx_fill(short *buf, size_t n_frames) {
    if (bounce_cooldown > 0)
        bounce_cooldown -= (int)n_frames;

    for (size_t i = 0; i < n_frames; i++) {
        float sample = 0.f;

        for (int v = 0; v < MAX_VOICES; v++) {
            Voice *vp = &voices[v];
            if (!vp->active) continue;

            float t = (float)vp->pos / (float)vp->len;

            /* Frequency: sweep or sequenced note */
            float freq;
            if (vp->num_notes > 0) {
                int ni = vp->pos / vp->note_len;
                if (ni >= vp->num_notes) ni = vp->num_notes - 1;
                freq = vp->note_freqs[ni];
            } else {
                freq = vp->freq_start + (vp->freq_end - vp->freq_start) * t;
            }

            /* Advance oscillator phase */
            vp->phase += freq / (float)SAMPLE_RATE;
            if (vp->phase >= 1.f) vp->phase -= 1.f;

            /* Generate sample */
            float osc;
            switch (vp->wave) {
                case 0: osc = sinf(vp->phase * 6.28318f); break;
                case 1: osc = (vp->phase < 0.5f) ? 1.f : -1.f; break;
                default: osc = ((float)rand() / (float)RAND_MAX) * 2.f - 1.f; break;
            }

            /* Envelope */
            float env;
            if (vp->num_notes > 0) {
                float note_t = (float)(vp->pos % vp->note_len) / (float)vp->note_len;
                env = 1.f - note_t * note_t;   /* quadratic decay per note */
            } else {
                env = 1.f - t;                  /* linear decay */
            }

            sample += osc * env * vp->volume;

            vp->pos++;
            if (vp->pos >= vp->len) vp->active = 0;
        }

        /* Mix in ambient music drones */
        for (int m = 0; m < MUSIC_VOICES; m++) {
            MusicVoice *mv = &music_voices[m];
            mv->lfo_phase += mv->lfo_rate / (float)SAMPLE_RATE;
            if (mv->lfo_phase >= 1.f) mv->lfo_phase -= 1.f;
            float freq = mv->base_freq + sinf(mv->lfo_phase * 6.28318f) * mv->lfo_depth;
            mv->osc_phase += freq / (float)SAMPLE_RATE;
            if (mv->osc_phase >= 1.f) mv->osc_phase -= 1.f;
            sample += sinf(mv->osc_phase * 6.28318f) * mv->volume;
        }

        /* Soft clip */
        if (sample >  1.f) sample =  1.f;
        if (sample < -1.f) sample = -1.f;

        short s = (short)(sample * 28000.f);
        buf[i * 2]     = s;
        buf[i * 2 + 1] = s;
    }
}

/* ------------------------------------------------------------------ */

void sfx_init(void) {
    memset(voices, 0, sizeof(voices));
    bounce_cooldown = 0;
    audio_init(SAMPLE_RATE, 4);
}

void sfx_poll(void) {
    while (audio_can_write()) {
        short *buf = audio_write_begin();
        sfx_fill(buf, (size_t)audio_get_buffer_length());
        audio_write_end();
    }
}
