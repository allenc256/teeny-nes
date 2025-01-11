# teeny-nes

A cross-platform Nintendo emulator, written using C++, [SDL2](https://github.com/libsdl-org/SDL), and [ImGui](https://github.com/ocornut/imgui). This emulator was written for entertainment purposes, a trip down memory lane...

# Features

* Full NTSC NES emulation, including cycle-accurate graphics (PPU) and sound (APU).
* A small (teeny?) and simple codebase --- straightforward code has been prioritized over all else.
* A relatively high level of emulation accuracy and game compatibility (see [Compatibility](#Compatibility) section below).
* Support for most major American-released games (see [Compatibility](#Unimplemented-Mappers)).
* Game Genie support (useful for testing!).

# Sample Screencasts

https://github.com/user-attachments/assets/3fba3c5e-378a-4864-a7f0-723719553716

https://github.com/user-attachments/assets/6399a7f2-2247-467e-a11b-3c4fcb684627

https://github.com/user-attachments/assets/6b893041-3a27-4018-8e71-4b0ec0c9d4be

# Building

Building this emulator requires:

* CMake (tested on 3.28.3)
* SDL2, e.g.
  - `sudo apt-get install libsdl2-dev` on Debian/Ubuntu
  - `brew install sdl2` on MacOS
* C++ compiler w/ C++20 support. The following have been tested:
  - GCC 13.3.0 (Ubuntu 24.04.1 LTS)
  - Clang 18.1.3 (Ubuntu 24.04.1 LTS)
  - Clang 15.0.0 (Apple Darwin)

Build using CMake (from root checkout directory):

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

Where the CMake build type should be as appropriate (e.g., `Debug` for debug builds). The above builds two executable targets:

* `teenynes` - this is the emulator application itself.
* `teenynes_test` - this is the emulator test suite.

# Controls

Only the keyboard and a single controller is supported.

| Controller Button | Keyboard Mapping |
| ----------------- | ---------------- |
| A                 | S                |
| B                 | A                |
| A Turbo           | X                |
| B Turbo           | Z                |
| Select            | Q                |
| Start             | W                |
| D-pad Up          | Up               |
| D-pad Down        | Down             |
| D-pad Left        | Left             |
| D-pad Right       | Right            |

# Compatibility

The following games have been tested (this is not a comprehensive list and the list of games which fully work is likely much longer):


| Game                          | Compatibility | Notes |
| ----------------------------- | ------------- | ----- |
| 1942                          | ✅            |       |
| Bionic Commando               | ✅            |       |
| Blades of Steel               | ✅            |       |
| Blaster Master                | ✅            |       |
| Castlevania                   | ✅            |       |
| Castlevania 2                 | ✅            |       |
| Contra                        | ✅            |       |
| Donkey Kong                   | ✅            |       |
| Double Dragon 2               | ✅            |       |
| Double Dragon 3               | ✅            |       |
| Faxanadu                      | ✅            |       |
| Final Fantasy                 | ✅            |       |
| Ghosts N Goblins              | ✅            |       |
| Gradius                       | ✅            |       |
| Guardian Legend               | ✅            |       |
| Ice Climber                   | ✅            |       |
| Ikari Warriors                | ✅            |       |
| Life Force                    | ✅            |       |
| Little Nemo: The Dream Master | ✅            |       |
| Marble Madness                | ✅            | Minor visual artifacts around text boxes at beginning of  certain levels. Fixing this requires tightening PPU/CPU timing, see https://www.nesdev.org/wiki/Tricky-to-emulate_games. |
| Mario                         | ✅            |       |
| Mario 2                       | ✅            |       |
| Mario 3                       | ✅            |       |
| Mega Man                      | ✅            |       |
| Mega Man 2                    | ✅            |       |
| Mega Man 3                    | ✅            |       |
| Metroid                       | ✅            |       |
| Ninja Gaiden                  | ✅            |       |
| Ninja Gaiden 2                | ✅            |       |
| Pinball                       | ✅            |       |
| Solstice                      | ✅            |       |
| Super Dodge Ball              | ✅            |       |
| Tetris                        | ✅            |       |
| TMNT                          | ✅            |       |
| TMNT 2                        | ✅            |       |
| Zelda                         | ✅            |       |
| Battletoads                   | ❌            | Crashes on 2nd level. Fixing this requires tightening CPU/PPU timing, as per https://www.nesdev.org/wiki/Tricky-to-emulate_games, https://www.nesdev.org/wiki/PPU_registers#Rendering_control, and https://forums.nesdev.org/viewtopic.php?t=6736). |

### Unimplemented Mappers

The majority of mappers for American-released Nintendo games have been implemented. In particular, these are iNES mappers 0 (NROM), 1 (MMC1), 2 (UxROM), 3 (CNROM), 4 (MMC3), and 7 (AxROM). There are large number of mappers which cover Japanese releases on the Famicom (notable examples would be the Konami VRC mappers) that are unsupported. There are two notable exceptions for American-released Nintendo game mappers that have not been implemented (yet):

* MMC2 - this mapper was built specifically for Mike Tyson's Punch-Out.
* MMC5 - the most notable game using this mapper is Castlevania 3, but there are a few other American releases using this mapper as noted here: https://nescartdb.com/search/advanced?ines=5.
* Additionally, only the most common variants of the mappers are implemented. For instance, MMC6 (technically a submapper of iNES mapper 4) is not supported.

# Technical Notes

* The 6502 CPU emulation is instruction-level (i.e., generally speaking, one emulation "step" processes one instruction), while the PPU and APU are emulated at cycle-level. PPU and APU cycles are synchronized to the CPU via [Catch-up](https://www.nesdev.org/wiki/Catch-up).
  - It would be an improvement to move the CPU emulation to cycle-level, but this would make the CPU emulation code significantly more complex, and it's unclear to me how much this would benefit compatibility.
  - Illegal opcodes are for the most part NOT implemented, as very few commercial games use them.
* Audio (APU) emulation follows the description given by Disch in the following nesdev.org forum post: https://forums.nesdev.org/viewtopic.php?f=3&t=13767.
  - Audio is synchronized to the video by *dynamically* adjusting the sampling rate up or down to try to maintain a constant-length audio queue. Rationale for this approach is described in https://forums.nesdev.org/viewtopic.php?f=3&t=11612.
* Graphics (PPU) emulation is cycle-level. For instance, PPU emulation is accurate enough to reproduce graphical glitches such as those described in https://www.youtube.com/watch?v=o9Ohvi10sM0. 
  - Emulation is implemented using C++20 coroutines, as they give a straightforward code representation of the somewhat complex state machine driving the PPU. The downside of the coroutine-based approach, however, is that save states would be harder to add to the emulator (as the contents of the coroutine frame are opaque / not ABI-stable across compilers).
