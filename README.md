![Logo2](https://github.com/Cyd0n1a/Superbounce64/blob/master/n64-logo.png?raw=true) # Superbounce64

![Logo](https://github.com/Cydonis-Heavy-Industries-C-H-I-Ltd/Cydonis-Heavy-Industries-C-H-I-Ltd.github.io/blob/main/chi-logo-only_transparent.png?raw=true) <br> <br>
<br>
![Screenshot](https://raw.githubusercontent.com/Cyd0n1a/Superbounce64/refs/heads/master/Screenshot_20260619_075921.png) <br><br>
An N64 game in the style of KBounce / Qix / Jezzball — claim territory by placing walls while avoiding bouncing balls. Built with [libdragon](https://github.com/DragonMinded/libdragon) & Tiny3D.<br>
![Logo3](https://github.com/Cyd0n1a/Superbounce64/blob/master/ld-logo.png?raw=true) <br>
## Gameplay

- Move your cursor around the unclaimed area with the **analog stick**.
- Press **A** to place a wall through your current position — it extends automatically in both directions until it hits existing walls.
- If a ball is in the path of a wall you're placing, you lose a life.
- Claim **75%** of the area to advance to the next level.
- Each level adds another ball and increases their speed.
- Try to get a high score! Scores *should* be saved to the controller pak, if one is plugged in/connected.
- Rumble pak support.

## Visual Style

Inspired by Jeff Minter's *Space Giraffe* — glowing wireframe graphics over a psychedelic palette-cycling plasma background, with a fake perspective grid overlay.

## Building

Requires Docker.

```bash
# Build the ROM
./build.sh

# Build all libdragon examples
./build-examples.sh
```

Produces `game.z64`, which can be run in any N64 emulator (e.g. [Ares](https://ares-emu.net/)) or flashed to a cartridge.

The build script pulls `ghcr.io/dragonminded/libdragon:latest` and installs libdragon + tools inside the container on each run.

## Controls

| Input | Action |
|-------|--------|
| Analog stick | Move cursor |
| A | Place wall |
| Start | Skip to gameplay (title screen) / Retry (game over) |

## Credits

**(c) 2026 Amanda Hariette-Scott & Cydonis Heavy Industries (C.H.I), Ltd.**

**https://cydonis.co.uk/about**
