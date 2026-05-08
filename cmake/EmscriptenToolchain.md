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

## Game Data (preloaded read-only assets)

The build system preloads game assets directly from the repository source
directories into the Emscripten virtual filesystem at build time.

| Source directory | Virtual FS path | Note |
|------------------|-----------------|------|
| `gamefiles/DATA`    | `/data_readonly` | seeded into `/data` on first run |
| `gamefiles/IMAGE08` | `/image08`       | read-only MEMFS |
| `gamefiles/IMAGE16` | `/image16`       | read-only MEMFS |
| `gamefiles/SOUND`   | `/sound`         | read-only MEMFS |

No manual copying of assets is required before building.

> **Case sensitivity**: The game's legacy code opens file paths in lower-case
> (e.g. `data/config.def`, `image08/...`). The virtual filesystem paths are
> set in lower-case in the CMake preload options accordingly.

## Persistent User Data (/data via IDBFS)

The Web build stores user-modified data (config, save-games, progress) in the
browser's **IndexedDB** via Emscripten's IDBFS, mounted at `/data`.

Game code uses relative paths like `data/config.def` which resolve to `/data/`
in the Emscripten virtual FS.  By mounting IDBFS directly at `/data`, **all
game reads and writes are automatically persistent** — no path changes in game
code are needed.

On startup `WebPersistence_Init()`:
1. Creates `/data` if absent.
2. Mounts IDBFS at `/data`.
3. Performs a blocking `FS.syncfs(true)` (via Asyncify) to populate the
   in-memory FS from IndexedDB before the game reads any config files.
4. Seeds any file present in `/data_readonly` but missing from `/data`
   (first-run initialization).
5. Flushes the seed writes back to IndexedDB with `FS.syncfs(false)`.

After every write of config/save data, `WebPersistence_SyncAsync()` triggers
a `FS.syncfs(false)` flush back to IndexedDB.

> **Clearing browser site data** will delete all IndexedDB entries and reset
> the game to its default state (re-seeded from `/data_readonly` on next load).

## Export and Import Save Data

Two C functions are exported from the Wasm binary and callable from JavaScript:

```js
// Download /save contents as free-eggbert-save.json
Module.ccall('FreeEggbert_ExportPersistentData', null, [], []);

// Open a file picker, import a previously exported .json into /save
Module.ccall('FreeEggbert_ImportPersistentData', null, [], []);
```

Export format: a JSON file `free-eggbert-save.json` containing a manifest of
all `/save` files with base64-encoded content.  Full ZIP support (e.g. JSZip)
is a planned future improvement.

After importing, reload the page to apply the restored save data.

## CMake Options

| Option | Default (Emscripten) | Description |
|--------|----------------------|-------------|
| `FREE_API_BUILD_TESTS` | `OFF` | Build free-api unit tests (not runnable in browser) |
| `FREE_DIRECT_DIAGNOSTICS` | `ON` | Enable Free Direct diagnostic counters |

Emscripten link flags added automatically for the game target:

| Flag | Purpose |
|------|---------|
| `-sASYNCIFY` | Allow blocking wait during IDBFS initial sync |
| `-sFORCE_FILESYSTEM=1` | Include full Emscripten FS support |
| `-sEXPORTED_RUNTIME_METHODS=['FS','ccall','cwrap']` | Expose FS runtime API and ccall/cwrap helpers to JS |
| `-sEXPORTED_FUNCTIONS=[...]` | Export `_main`, `_FreeEggbert_ExportPersistentData`, `_FreeEggbert_ImportPersistentData` |
| `-sALLOW_MEMORY_GROWTH=1` | Allow heap to grow at runtime |

## Architecture Notes

- `if(EMSCRIPTEN)` is used throughout CMakeLists.txt files to gate
  Web-specific settings.
- SDL is built as a **static** library under Emscripten (shared libraries are
  not supported in WebAssembly).
- `WaitMessage()` is disabled on Emscripten (`#if !defined(__EMSCRIPTEN__)`)
  to prevent blocking the browser main thread.
- `web_persistence.cpp` is compiled only when `__EMSCRIPTEN__` is defined;
  the file is a no-op empty translation unit on native builds.
- Native Linux and Windows builds are completely unaffected by all persistence
  code — `WebPersistence_Init()` and friends are inline no-ops on non-Web
  platforms.
