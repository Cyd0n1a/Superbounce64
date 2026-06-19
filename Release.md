# Release Notes

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
- Built with [libdragon](https://github.com/DragonMinded/libdragon) (preview toolchain)
- Docker-based build system — no local N64 toolchain required
- Fixed 60 Hz timestep game loop with real-time visual update
- libdragon included as a git submodule

### Known Issues

- Cursor can occasionally clip into narrow 1-cell corridors between walls
- No music (procedural SFX only)
- No high score persistence
- No pause menu
