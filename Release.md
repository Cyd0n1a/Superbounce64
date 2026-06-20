# Release Notes

## v1.0 — 2026

### Gameplay
- **Gradual wall building** — pressing A now locks the cursor in place and extends the wall outward from both sides simultaneously at 50 cells per second, rather than placing it instantly. Balls bounce off already-placed sections in real time. If a ball occupies a cell at the moment a wall head reaches it, the entire in-progress wall is erased and a life is lost — matching the core tension of KBounce. The wall completes and claims territory only once both heads connect to a boundary.

---

## v0.9b — 2026

### Fixes
- **Title screen text flickering on real hardware** — `rdpq_detach()` queues a SYNC_FULL to the RDP but returns immediately without blocking. On real hardware the RDP was still actively writing the frame's background and plasma overlay when the CPU began `graphics_draw_character()` calls, producing corrupted and flickering text. `rspq_wait()` is now called after `rdpq_detach()` to block until the SYNC_FULL interrupt fires, making the CPU/RDP handoff race-free. Not visible on emulators, which lack true hardware concurrency.
- **Perspective grid rendering** — `draw_grid_overlay()` was inheriting the wrong RDP mode (`RDPQ_COMBINER_TEX_FLAT`) left over from the plasma texture draw. Grid lines now set `RDPQ_COMBINER_SHADE` + `RDPQ_BLENDER_MULTIPLY` explicitly, so per-vertex dimming renders correctly instead of sampling a fixed texture pixel.

---

## v0.9a — 2026

### Performance
- **Claimed territory fill** — replaced per-cell `rdpq_fill_rectangle` (up to 3,600 RDP commands at 75% claimed) with a run-length scan that emits one rectangle per row of consecutive claimed cells (at most 60). This was the primary cause of lag worsening as levels progressed.
- **Palette cycling** — precompute the 256-entry rainbow palette once at startup; each frame now does two `memcpy` calls to rotate it by an index offset instead of recomputing 256 HSV→RGB conversions.
- **RDP mode setup** — `rdpq_set_mode_standard` + combiner + blender are now called once per draw group (walls, balls, cursor) rather than once per wall segment or ball edge. The breathing-pulse `sinf` is also computed once per frame and passed through, not evaluated per segment.

---

## v0.8b — 2026

### Fixes
- **Music now plays** — `dfs_open()` takes a bare filename without the `rom:/` prefix; passing the full path caused a silent error that prevented `xm64player_open` from ever being called.

---

## v0.8a — 2026

### New Features

**Gameplay**
- **Pause menu** — press Start during play to pause; press Start again to resume. Game state is fully preserved.
- **Wall preview line** — a ghost line shows exactly where your next wall will extend before you commit. Cyan when the path is clear, red when a ball is blocking it. Axis locks to the dominant stick direction and updates every frame.
- **Score system** — each wall placement scores `territory_gained% × level × 10` points. Score accumulates across levels and resets on a new game. Current score and session best are shown in the HUD during play.

**Audio**
- **Mozart piano music** — looping XM module music (`mozartku.xm`) played back via libdragon's RSP hardware mixer (`xm64player`). Runs entirely off the RSP with no CPU overhead. Coexists with procedural SFX via saturating-add mixing. Music is embedded in the ROM filesystem (DFS) and converted at build time by `audioconv64`.
- **Rumble Pak support** — brief tap on successful wall placement, sustained buzz on a life lost, medium pulse on level complete. Requires a Rumble Pak in controller port 1 (configure in emulator accessory settings).

**Saving**
- **Controller Pak high score persistence** — best score is saved to a Controller Pak note (`SUPERBOUNCE`) and loaded automatically during the title screen. Saved best score displayed in gold on the title screen. Gracefully skips saving if no Controller Pak is present or the pak is full. Requires a Controller Pak in controller port 1 (note: cannot use Rumble Pak and Controller Pak simultaneously).

**Visuals**
- **Border / wall distinction** — the field boundary now renders as a thin dim blue line, distinct from the bright cyan glow of walls you place. Makes it immediately clear what you built vs what was already there.
- **Improved ball spawning** — balls spread across a grid at level start (2 columns for ≤4 balls, 3 columns for 5–8) with evenly distributed launch angles. Prevents clustering on one side of the field.
- **New high score flash** — blinking gold "NEW HIGH SCORE!" text on the game over screen when you beat your best.

---

## v0.5a — Initial Release (2026)

First public release of Superbounce64.

### Features

**Gameplay**
- KBounce / Qix-style territory claiming on a 320×240 grid
- Wall placement along horizontal or vertical axes, extending automatically to the nearest boundary
- Flood-fill claims the smaller enclosed region that contains no balls
- 75% claimed area required to advance each level
- 3 lives; losing all lives ends the game
- Infinite levels — each adds one more ball and increases speed by 15%
- Up to 8 balls in play simultaneously

**Visuals**
- Psychedelic animated background: Gouraud-shaded corner gradient + CI8 plasma texture with per-frame palette cycling
- Fake perspective wireframe grid overlay (convergent vertical lines, quadratic horizontal spacing)
- Glowing wireframe aesthetic for walls, balls, and cursor — 3-layer multiply-alpha glow effect
- Pulsing wall glow breathing at ~4 Hz
- Balls rendered as animated wireframe hexagons in 8 neon colours
- Title screen with wavy per-character rainbow sine animation on the game title
- Blinking PRESS START prompt on title screen

**Audio**
- Fully procedural sound effects (no sample data):
  - Wall placement: descending sine frequency sweep (900 → 150 Hz)
  - Wall fail: square wave buzz
  - Level complete: ascending C–E–G sine arpeggio
  - Ball bounce: filtered noise tick (rate-limited to prevent flooding)

**Tech**
- Built with [libdragon](https://github.com/DragonMinded/libdragon)
- Docker-based build system — no local N64 toolchain required
- Fixed 60 Hz timestep game loop with real-time visual update
- libdragon included as a git submodule

### Known Issues (resolved in v0.7a–v0.8a)
- No music (replaced with Mozart piano XM in v0.8a)
- No high score persistence (resolved in v0.7a)
- No pause menu (resolved in v0.7a)
