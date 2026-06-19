# Release Notes

## v0.7a — 2026

### New Features

**Gameplay**
- **Pause menu** — press Start during play to pause; press Start again to resume. Game state is fully preserved.
- **Wall preview line** — a ghost line shows exactly where your next wall will extend before you commit. Cyan when the path is clear, red when a ball is blocking it. Axis locks to the dominant stick direction and updates every frame.
- **Score system** — each wall placement scores `territory_gained% × level × 10` points. Score accumulates across levels and resets on a new game. Current score and session best are shown in the HUD during play.

**Audio**
- **Ambient music** — continuous procedural drone: three sine voices forming an Am chord (A2 / C3 / E3) with independent slow LFOs that drift the pitch gently, giving a breathing psychedelic quality throughout play and the title screen.
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

### Known Issues (resolved in v0.7a)
- No music
- No high score persistence
- No pause menu
