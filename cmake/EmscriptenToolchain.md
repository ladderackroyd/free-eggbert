# Building for Web with Emscripten

This document describes how to build **Speedy Blupi / Speedy Eggbert** as a
WebAssembly application using [Emscripten](https://emscripten.org/).

## Prerequisites

1. Install and activate the Emscripten SDK (emsdk):

   ```sh
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh
   ```

2. All Git submodules must be checked out (vendored SDL, SDL_image, SDL_mixer):

   ```sh
   git submodule update --init --recursive
   ```

## Configure and Build

From the repository root:

```sh
source /path/to/emsdk/emsdk_env.sh
emcmake cmake -S . -B cmake-build-web -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-web -j
```

The output files will be placed in `cmake-build-web/bin/`:

| File | Description |
|------|-------------|
| `SPEEDY_BLUPI_WINDOWS.html` | Shell page that loads the game |
| `SPEEDY_BLUPI_WINDOWS.js`   | Emscripten glue JavaScript |
| `SPEEDY_BLUPI_WINDOWS.wasm` | Compiled WebAssembly binary |
| `SPEEDY_BLUPI_WINDOWS.data` | Preloaded virtual filesystem image |

## Running Locally

Use `emrun` (part of the emsdk) to serve the output over HTTP — browsers
block loading local `.wasm`/`.data` files from `file://` URLs:

```sh
emrun cmake-build-web/bin/SPEEDY_BLUPI_WINDOWS.html
```

## Game Data

The build system preloads game assets directly from the repository source
directories into the Emscripten virtual filesystem:

| Source directory | Virtual FS path |
|------------------|-----------------|
| `gamefiles/DATA`    | `/data`    |
| `gamefiles/IMAGE08` | `/image08` |
| `gamefiles/IMAGE16` | `/image16` |
| `gamefiles/SOUND`   | `/sound`   |

No manual copying of assets is required before building.

> **Case sensitivity**: The game's legacy code opens file paths in lower-case
> (e.g. `data/config.def`, `image08/...`). The virtual filesystem paths are
> set in lower-case in the CMake preload options accordingly.

## CMake Options

| Option | Default (Emscripten) | Description |
|--------|----------------------|-------------|
| `FREE_API_BUILD_TESTS` | `OFF` | Build free-api unit tests (not runnable in browser) |
| `FREE_DIRECT_DIAGNOSTICS` | `ON` | Enable Free Direct diagnostic counters |

## Architecture Notes

- `if(EMSCRIPTEN)` is used throughout CMakeLists.txt files to gate
  Web-specific settings.
- SDL is built as a **static** library under Emscripten (shared libraries are
  not supported in WebAssembly).
- `WaitMessage()` is disabled on Emscripten to prevent blocking the
  browser main thread.
