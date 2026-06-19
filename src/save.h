#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>

/* Try to load saved high score from Controller Pak.
   Safe to call every frame — no-ops once loaded or if pak absent. */
void save_try_load(void);

/* Write a new high score to the Controller Pak.
   Silently skips if no Controller Pak is present or pak is full. */
void save_write_high_score(uint32_t score);

/* Returns the loaded high score (0 if no pak or no save found yet). */
uint32_t save_get_high_score(void);

#endif
