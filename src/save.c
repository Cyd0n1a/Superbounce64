#include "save.h"
#include <joypad.h>
#include <mempak.h>
#include <string.h>

#define SAVE_MAGIC   0x53423634u   /* 'SB64' */
#define SAVE_NOTE    "SUPERBOUNCE"
#define SAVE_GAME_ID 0x5342        /* 'SB' */

typedef struct {
    uint32_t magic;
    uint32_t high_score;
    uint32_t checksum;  /* magic XOR high_score */
} SaveData;

static int      save_loaded     = 0;
static uint32_t cached_hi_score = 0;

/* Find our note entry; returns 1 and fills *out on success. */
static int find_entry(entry_structure_t *out) {
    for (int i = 0; i < 16; i++) {
        entry_structure_t e;
        if (get_mempak_entry(0, i, &e) == 0 && e.valid &&
            strcmp(e.name, SAVE_NOTE) == 0 && e.game_id == SAVE_GAME_ID) {
            *out = e;
            return 1;
        }
    }
    return 0;
}

void save_try_load(void) {
    if (save_loaded) return;
    if (joypad_get_accessory_type(JOYPAD_PORT_1) != JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) return;
    if (validate_mempak(0) != 0) { save_loaded = 1; return; }

    entry_structure_t entry;
    if (find_entry(&entry)) {
        uint8_t buf[MEMPAK_BLOCK_SIZE];
        if (read_mempak_entry_data(0, &entry, buf) == 0) {
            SaveData *sd = (SaveData *)buf;
            if (sd->magic == SAVE_MAGIC &&
                sd->checksum == (sd->magic ^ sd->high_score)) {
                cached_hi_score = sd->high_score;
            }
        }
    }
    save_loaded = 1;
}

void save_write_high_score(uint32_t score) {
    if (joypad_get_accessory_type(JOYPAD_PORT_1) != JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) return;
    if (validate_mempak(0) != 0) return;

    /* Delete old entry so we can rewrite it */
    entry_structure_t old;
    if (find_entry(&old))
        delete_mempak_entry(0, &old);

    uint8_t buf[MEMPAK_BLOCK_SIZE];
    memset(buf, 0, sizeof(buf));
    SaveData *sd = (SaveData *)buf;
    sd->magic      = SAVE_MAGIC;
    sd->high_score = score;
    sd->checksum   = sd->magic ^ sd->high_score;

    entry_structure_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.vendor  = 0;
    entry.game_id = SAVE_GAME_ID;
    entry.region  = 0;
    entry.blocks  = 1;
    strncpy(entry.name, SAVE_NOTE, sizeof(entry.name) - 1);

    if (write_mempak_entry_data(0, &entry, buf) == 0)
        cached_hi_score = score;
}

uint32_t save_get_high_score(void) {
    return cached_hi_score;
}
