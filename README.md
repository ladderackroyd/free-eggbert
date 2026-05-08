# Free Eggbert

![Decompilation gameplay screenshot from October 29, 2024](screenshot.png)

## Introduction

**Free Eggbert** is a reconstruction project for **Speedy Eggbert 2**, based on decompiled and reverse-engineered source code.

The original source code for **Speedy Blupi / Speedy Eggbert** has not been publicly released. This project uses **Ghidra** and **IDA** to inspect the original game binaries, while also comparing behavior and structure against the official **Planet Blupi** source code.

The current goal is to make the game buildable, playable, documented, and eventually portable beyond its original Windows/DirectX environment.

## Current Status

Gameplay is already partially functional, but the game is still defective and many features are incomplete, inaccurate, or missing.

This repository should be considered a work-in-progress reverse-engineering and preservation effort.

## Used Technologies

- C++
- DirectX 3
- Visual Studio 2022
- Ghidra
- IDA
- ILSpy

## Goals

- [ ] Improve accuracy of the decompiled source code
- [ ] Restore missing or defective gameplay behavior
- [ ] Add Doxygen documentation
- [ ] Add WineLib support via CMake
- [ ] Add support for Free Direct via CMake
- [ ] Make the game portable to additional platforms
- [ ] Investigate Android support
- [ ] Investigate web browser support

### Features, which are not yet implemented

### Features, which has issues

## Development Notes

Some demangled symbol names are taken from the 2013 Windows Phone version of **Speedy Blupi**, inspected using ILSpy.

Special thanks to **Ч.У.Ш** from the 4PDA forum for archiving this obscure version of the game.

For personal convenience, this code currently uses the **BASS** and **BASSMIDI** audio libraries.  
The original Windows MCI-based behavior can be restored in `def.h` by changing:
 - `#define _BASS` to: `#define _BASS false`

### Files Requiring the Most Attention

These files currently need the most work, listed in rough priority order:

* `event.cpp`
* `decblupi.cpp`
* `decio.cpp`
* `decdesign.cpp`
* `decblock.cpp`
* `decmove.cpp`
* `decnet.cpp`
* `decor.cpp`
* `misc.cpp`

### Files Tentatively Complete

These files appear mostly complete, but still require testing and verification:

* `blupi.cpp`
* `button.cpp`
* `jauge.cpp`
* `menu.cpp`
* `movie.cpp`
* `network.cpp`
* `pixmap.cpp`
* `sound.cpp`
* `wave.cpp`

## Local Setup

### Clone the Repository

After cloning the repository, initialize and update submodules:

```bash
git submodule init
git submodule update
```

Alternatively:

```bash
git submodule update --init --recursive
```

This project vendors SDL via git submodules under `third_party/`:

- `third_party/SDL`
- `third_party/SDL_image`
- `third_party/SDL_mixer`

By default, CMake builds these vendored dependencies directly (no system SDL packages required).

Optional override for advanced users:

```bash
cmake -S . -B build -DFREE_USE_SYSTEM_SDL=ON
```

### Add game files of Speedy Eggbert 2
 
 - DATA
 - IMAGE08
 - IMAGE16
 - SOUND

## Development Environment - Visual Studio

### Requirements

* Microsoft Visual Studio Community 2022
* MSVC v143 desktop toolset
* Windows x86 build target

### Open the Project

 - Open the solution file: `Speedy Eggbert 2 Source.sln`
 - Use **Microsoft Visual Studio 2022**.
 - Set the platform to: `x86`
 - Set the debugger target to: `Win32`

### Set Additional Compiler Options

Open:

Project Properties
 - Configuration Properties
 - C/C++
 - Command Line
 - Additional Options


Add: `/wd4700 /wd4703`

Note: These options disable warnings related to potentially uninitialized local variables.

### Set Platform Toolset

Open:

Project Properties
 - General
 - Platform Toolset

Set it to: `Visual Studio 2022 (v143)`

### Build

Right-click the solution: `Speedy Eggbert 2 Source.sln`

Then select: `Build Solution`

## Development Environment - CLion

### WineLib

Install: `apt install libwine-dev libjack-dev`  
Build via console: 
```bash
git submodule update --init --recursive
cmake -S . -B build -DSPEEDY_BLUPI_BACKEND=FREEDIRECT
cmake -B build \
  -DSPEEDY_BLUPI_BACKEND=WINELIB \
  -DCMAKE_CXX_COMPILER=wineg++-stable \
  -DCMAKE_C_COMPILER=winegcc-stable

cmake --build build
```

If an older build directory cached system SDL package paths, do a clean reconfigure:
#### Clion

```bash
rm -rf build
git submodule update --init --recursive
cmake -S . -B build -DSPEEDY_BLUPI_BACKEND=FREEDIRECT
cmake --build build
File > Settings > Build, Execution, Deployment > CMake  
Add CMake profile and add:

```
-DCMAKE_CXX_COMPILER=wineg++-stable
-DCMAKE_C_COMPILER=winegcc-stable
-DCMAKE_CXX_FLAGS="-m64"
```

Windows (Visual Studio generator example):
Edit Run configuration:

```bash
git submodule update --init --recursive
cmake -S . -B build -G "Visual Studio 17 2022" -DSPEEDY_BLUPI_BACKEND=FREEDIRECT
cmake --build build --config Debug
```
Executable=/usr/bin/wine
Arguments={PATH}/winelib-demo/cmake-build-debug/bin/WINELIB_DEMO.exe
```

Dependency boundary note:

- `SPEEDY_BLUPI_WINDOWS` links only `free-api` and `free-direct`.
- SDL include directories, compile definitions, and SDL libraries stay private to `free-api` / `free-direct`.
## Known Workaround

This workaround may be required in some configurations:

```cpp
typedef struct IUnknown IUnknown;
```

It is used to avoid the following error:

```text
c:\program files (x86)\windows kits\8.1\include\um\combaseapi.h(229):
error C2760: syntax error: unexpected token 'identifier', expected 'type specifier'
```

## Disclaimer

This project is intended for research, preservation, documentation, and compatibility work.

It is not an official release of the original game source code.
