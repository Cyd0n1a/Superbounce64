# Superbounce64 Cloud Agent Instructions

## Repository Overview

**Superbounce64** is an N64 game built with [libdragon](https://github.com/DragonMinded/libdragon), inspired by KBounce/Qix. Players claim territory on a 320×240 grid by placing walls while avoiding bouncing balls. The game features glowing wireframe graphics, psychedelic plasma backgrounds, hardware 3D rendered title screen, Mozart piano music, and full Rumble Pak support.

**Type:** N64 game  
**Languages:** C  
**Frameworks:** libdragon (N64 SDK), tiny3d (3D graphics microcode)  
**Output:** Z64 ROM file (playable in emulators or on real cartridges)  
**Build System:** Docker-based (requires Docker; no local N64 toolchain needed)

## Build Instructions

### Prerequisites
- **Docker** is required and must be running
- Both `libdragon` and `tiny3d` are Git submodules — they are pulled from remote repositories during the build

### Building the Game

**Always use this command to build the ROM:**

```bash
./build.sh
```

This script:
1. Pulls the latest `ghcr.io/dragonminded/libdragon:latest` Docker image
2. Installs libdragon and tools inside the container
3. Builds the tiny3d graphics library
4. Compiles all source files and the audio asset
5. Links the ROM as `game.z64`

**Build time:** ~2–3 minutes on first run (Docker image pull), ~30–60 seconds on subsequent runs.

### Cleaning and Rebuilding

```bash
# Clean generated build artifacts
make clean

# Then rebuild
./build.sh
```

**Important:** Always run `make clean` before building if you've made significant changes or suspect stale object files. The `build.sh` script does NOT automatically clean before building.

### Build Artifacts

- **`game.z64`** — The compiled N64 ROM (runnable in emulators)
- **`build/` directory** — Object files, dependency files, ELF intermediate
- **`filesystem/` directory** — Converted audio file

## Project Layout

### Source Code (`src/`)
All game logic is in C:
- **`main.c`** — Entry point; initializes libdragon, runs the game loop with fixed 60 Hz timestep (~16.667 ms per frame)
- **`game.c/h`** — Core game state and level progression
- **`field.c/h`** — Territory grid management and wall/flood-fill logic
- **`player.c/h`** — Player cursor control and wall placement
- **`balls.c/h`** — Ball physics, collision detection, and movement
- **`render.c/h`** — Graphics rendering (wireframe walls, balls, grid overlay, plasma)
- **`effects.c/h`** — Visual effects (palette cycling, glow animations)
- **`title3d.c/h`** — 3D-rendered title screen with rotating sphere
- **`play3d.c/h`** — 3D background rendering during gameplay
- **`sfx.c/h`** — Procedural sound effects (no samples; all sine/square/noise)
- **`save.c/h`** — Controller Pak high score persistence

### Build Configuration
- **`Makefile`** — Defines compilation rules, links object files, builds ROM
  - Includes `$(N64_INST)/include/n64.mk` (libdragon N64 build rules)
  - Includes `tiny3d/t3d.mk` (tiny3d 3D library build rules)
  - Target: `all` → `game.z64`
  - Target: `clean` → removes `build/`, `game.z64`, `filesystem/`

- **`build-examples.sh`** — Builds libdragon example programs (not used for the game)

### Assets
- **`assets/mozartku.xm`** — XM tracker module (Mozart piano music); converted to `filesystem/mozartku.xm64` at build time

### Dependencies (Git Submodules)
- **`libdragon/`** — N64 SDK with compiler, graphics library (RDP/RSP), audio, controller/save systems
- **`tiny3d/`** — Hardware 3D graphics microcode for the RSP

### Root Level Files
- **`.gitignore`** — Excludes build artifacts and ROM files
- **`.gitmodules`** — Defines submodule remotes and paths
- **`README.md`** — User-facing documentation
- **`Release.md`** — Version history and changelog
- **`LICENSE.md`** — GPL v2 license

## Configuration & Dependencies

### Hidden Dependencies
- **Docker image `ghcr.io/dragonminded/libdragon:latest`** is pulled on every build. Network issues during image pull will fail the build.
- **Git submodules** are not automatically initialized. However, `build.sh` runs `make -C libdragon install` which handles submodule setup during build.
- **C standard library:** Uses libdragon's standard C library with N64 MIPS architecture specifics.

### Important Macros & Constants
Located in `src/main.c`:
- **`STEP_US` = 16667** — Fixed timestep in microseconds (60 Hz). All game logic runs at this frequency.

## Validation & Testing

### Running the Built ROM
After `./build.sh` completes successfully, `game.z64` can be:
1. **Run in an emulator** (e.g., [Ares](https://ares-emu.net/), Mupen64Plus)
2. **Flashed to a real N64 cartridge** (requires cartridge hardware)

### Manual Validation Steps
1. **After building**, verify `game.z64` exists and is non-zero in size (typically ~512 KB–1 MB)
2. **Run in an emulator** to confirm:
   - Title screen displays with rotating 3D sphere and wavy text
   - Mozart music plays (if audio is enabled in emulator)
   - Game starts on A or Start button
   - Gameplay: cursor moves with analog stick, A places walls, walls extend until hitting boundaries
   - Score, lives, and level display correctly
   - Rumble feedback (if supported; may not work in all emulators)

### No Automated Tests
This repository does not include unit tests or CI/CD pipelines. All validation is manual via emulator gameplay.

## Common Pitfalls & Workarounds

### Docker Image Pull Failures
**Problem:** Build fails with "image not found" or network error.  
**Solution:** Ensure Docker is running and you have internet access. Retry `./build.sh`.

### Stale Object Files
**Problem:** After editing a header file, the build doesn't pick up changes.  
**Solution:** Run `make clean` before `./build.sh`.

### Submodule Issues
**Problem:** Build fails with errors about libdragon or tiny3d paths not found.  
**Solution:** Git submodules are initialized during build by `build.sh`. If this fails, manually run:
```bash
git submodule update --init --recursive
```
Then retry `./build.sh`.

### Audio Not Converting
**Problem:** Build complains about missing `filesystem/mozartku.xm64`.  
**Solution:** This file is auto-generated from `assets/mozartku.xm` during the build. Ensure `assets/mozartku.xm` exists and is not corrupted. If all else fails, clean and rebuild:
```bash
make clean
./build.sh
```

## Making Code Changes

### Where to Add Features
- **Gameplay logic:** `src/game.c`, `src/player.c`, `src/field.c`, `src/balls.c`
- **Rendering:** `src/render.c`
- **Audio/SFX:** `src/sfx.c`
- **3D elements:** `src/title3d.c`, `src/play3d.c`
- **Controller/save data:** `src/player.c` (controls), `src/save.c` (persistence)

### Building After Changes
```bash
./build.sh
```
The Makefile automatically detects source file changes via dependency tracking (`.d` files) and recompiles only what's needed. If compilation fails, check error output carefully—linker errors often indicate missing includes or undefined symbols.

### Validation After Changes
1. Run `./build.sh`
2. Test in an emulator to verify gameplay behavior
3. Check console output in the emulator for any debug messages (if added)

## Important Notes

- **Always trust these instructions.** Only perform additional searches if the information here is incomplete or found to be in error.
- **Docker is mandatory** — you cannot build the ROM without Docker installed and running.
- **Fixed 60 Hz timestep** — game logic runs at precisely 16.667 ms per frame; timing-sensitive code must respect this.
- **libdragon is actively maintained** — the build pulls the latest image, which may occasionally introduce breaking changes. If builds suddenly fail after no code changes, check the libdragon repository for recent updates.
- **No test suite** — validation is manual via emulator. Always test in an emulator before considering a change complete.
