#pragma once

// web_persistence.hpp
//
// Emscripten/Web-only persistent storage support using IDBFS.
// All functions are no-ops on native builds.
//
// Architecture:
//   /data_readonly               -- preloaded read-only game assets (MEMFS)
//   /data                        -- IDBFS-backed, writable; seeded from /data_readonly on first run
//   /image08, /image16, /sound   -- preloaded read-only assets; never touched by persistence layer
//
// Game code uses relative paths like "data/config.def" which resolve to /data/... in the
// Emscripten virtual FS.  By mounting IDBFS at /data, all game reads and writes go through
// IndexedDB automatically — no path changes in game code are needed.
//
// On startup call WebPersistence_Init() before reading any user config/save
// files.  After the game writes config or save data call WebPersistence_SyncAsync()
// so changes are flushed to the browser's IndexedDB store.

#if defined(__EMSCRIPTEN__)

/// Mount IDBFS at /data, perform an initial synchronous population from
/// IndexedDB into the in-memory virtual FS, then seed any missing files from
/// /data_readonly so the game finds its default assets on first run.
///
/// Must be called before DoInit() / ReadConfig() so that previously-saved
/// data is visible to the game.
///
/// Returns true on success, false if the mount or initial sync failed.
bool WebPersistence_Init();

/// Asynchronously flush the in-memory /data contents back to IndexedDB.
/// Call this after every successful write of config/save/progress data.
/// Safe to call frequently — the browser batches the writes.
void WebPersistence_SyncAsync();

/// Best-effort blocking-ish flush.  Uses Asyncify if available; falls back to
/// an async flush otherwise.  Suitable for use at shutdown / before page unload.
void WebPersistence_SyncBlocking();

// ---- Export / Import -------------------------------------------------------

/// Download the contents of /data as a JSON manifest named "free-eggbert-save.json"
/// (base64-encoded file contents).  Exposed to JavaScript via Emscripten
/// EXPORTED_FUNCTIONS so it can be called from a browser button or the console:
///   Module.ccall('FreeEggbert_ExportPersistentData', null, [], []);
extern "C" void FreeEggbert_ExportPersistentData();

/// Accept a previously-exported JSON manifest from a browser file-picker,
/// restore the files into /data, and flush to IndexedDB.
/// Exposed to JavaScript the same way as the export function.
extern "C" void FreeEggbert_ImportPersistentData();

#else // !__EMSCRIPTEN__ -------------------------------------------------------

// On native builds every function is an inline no-op so that call-sites in
// shared code (e.g. blupi.cpp) compile without any platform guards.

inline bool WebPersistence_Init()       { return true; }
inline void WebPersistence_SyncAsync()  {}
inline void WebPersistence_SyncBlocking() {}

extern "C" inline void FreeEggbert_ExportPersistentData() {}
extern "C" inline void FreeEggbert_ImportPersistentData() {}

#endif // __EMSCRIPTEN__
