# teeny-nes

A cross-platform Nintendo emulator, written using C++, [SDL2](https://github.com/libsdl-org/SDL), and [ImGui](https://github.com/ocornut/imgui). This emulator was written for entertainment purposes, a trip down memory lane.

# Features

* Full NTSC NES emulation, including cycle-accurate graphics (PPU) and sound (APU).
* A small (teeny?) and simple codebase --- straightforward code has been prioritized over all else.
* A relatively high level of emulation accuracy and game compatibility (see [Compatibility](#Compatibility) section below).
* Support for most major American game mappers (see [Compatibility](#Unimplemented-Mappers) for non-emulated games).
* Game Genie support (useful for testing!).

# Building

Building this emulator requires:

* CMake (tested on 3.28.3)
* SDL2, e.g.
  - `sudo apt-get install libsdl2-dev` on Debian/Ubuntu
  - `brew install sdl2` on MacOS
* C++ compiler w/ C++20 support. The following have been tested:
  - GCC 13.3.0 (Ubuntu 24.04.1 LTS)
  - Clang 18.1.3 (Ubuntu 24.04.1 LTS)

Build using CMake (from root checkout directory):

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

Where the CMake build type should be as appropriate (e.g., `Debug` for debug builds).

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
| Battletoads                   | ❌            | Crashes on 2nd level. Fixing this requires tightening CPU/PPU timing, as per https://www.nesdev.org/wiki/Tricky-to-emulate_games and https://forums.nesdev.org/viewtopic.php?t=6736). |

### Unimplemented Mappers

The majority of mappers for American-released Nintendo games have been implemented. There are large number of mappers which cover Japanese releases on the Famicom (such as, the Konami VRC mappers) that are unsupported. There are two notable exceptions for American-released Nintendo game mappers that have not been implemented (yet):

| Mapper | Notes |
| ------ | ----- |
| MMC2   | This mapper was built specifically for Mike Tyson's Punch-Out. |
| MMC5   | The most notable game using this mapper is Castlevania 3, but there are a few other American releases using this mapper as noted here: https://nescartdb.com/search/advanced?ines=5 |
