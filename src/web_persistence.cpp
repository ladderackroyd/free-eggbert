// web_persistence.cpp
//
// Emscripten/Web persistent storage support — compiled only for __EMSCRIPTEN__.
//
// Design:
//   Game code opens relative paths like "data/config.def" which resolve to
//   /data/... in the Emscripten virtual FS.  To make those writes persistent
//   across browser reloads we mount IDBFS directly at /data.
//
//   Read-only game assets are preloaded by the build system into /data_readonly.
//   On first run (IDBFS is empty) WebPersistence_Init() seeds /data from
//   /data_readonly so the game finds its default files.  On subsequent runs the
//   IDBFS-backed /data already contains the user's saved data and the seed step
//   only copies files that are genuinely missing.
//
//   /image08, /image16, /sound are preloaded directly from source and are never
//   touched by the persistence layer.
//
// Startup ordering (called from WinMain before DoInit):
//   1. WebPersistence_Init()
//      - mkdir /data if absent
//      - mount IDBFS at /data
//      - FS.syncfs(true, ...) — populate memory from IndexedDB (Asyncify-blocking)
//      - seed any missing files from /data_readonly into /data
//      - FS.syncfs(false, ...) — flush seed to IndexedDB
//
// After any game write that needs flushing:
//   WebPersistence_SyncAsync()  — FS.syncfs(false, ...) async flush to IDB.

#if defined(__EMSCRIPTEN__)

#include "web_persistence.hpp"

#include <emscripten/emscripten.h>
#include <cstdio>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/// Create /data directory if it does not exist (ignore EEXIST).
static void ensure_data_dir()
{
    EM_ASM({
        try { FS.mkdir('/data'); } catch(e) { /* already exists */ }
    });
}

/// Mount IDBFS at /data.  Returns false if mount fails.
static bool mount_idbfs()
{
    int ok = EM_ASM_INT({
        try {
            FS.mount(IDBFS, {}, '/data');
            return 1;
        } catch(e) {
            console.error('WebPersistence: IDBFS mount failed: ' + e.message);
            return 0;
        }
    });
    return ok != 0;
}

/// Blocking syncfs(populate) via EM_ASYNC_JS — correctly suspends the C
/// call stack via Asyncify until the IndexedDB callback fires.
EM_ASYNC_JS(int, idbfs_syncfs_blocking, (int populate), {
    return await new Promise(function(resolve) {
        FS.syncfs(populate ? true : false, function(err) {
            if (err) {
                console.error('WebPersistence: syncfs failed: ' + err);
                resolve(0);
            } else {
                resolve(1);
            }
        });
    });
});

/// Seed any file that exists in /data_readonly but not yet in /data.
/// This handles first-run initialization and ensures new game assets added
/// in updates are visible even when IndexedDB already has older data.
static void seed_from_readonly()
{
    EM_ASM({
        var copied = 0;
        var skipped = 0;
        function seedDir(srcDir, dstDir) {
            var entries;
            try { entries = FS.readdir(srcDir); } catch(e) {
                console.error('WebPersistence seed: cannot readdir ' + srcDir + ': ' + e.message);
                return;
            }
            for (var i = 0; i < entries.length; i++) {
                var name = entries[i];
                if (name === '.' || name === '..') continue;
                var src = srcDir + '/' + name;
                var dst = dstDir + '/' + name;
                var stat;
                try { stat = FS.stat(src); } catch(e) { continue; }
                if (FS.isDir(stat.mode)) {
                    try { FS.mkdir(dst); } catch(e) {}
                    seedDir(src, dst);
                } else {
                    // Only copy if the destination does not exist yet.
                    var exists = false;
                    try { FS.stat(dst); exists = true; } catch(e) {}
                    if (!exists) {
                        try {
                            var data = FS.readFile(src);
                            FS.writeFile(dst, data);
                            copied++;
                        } catch(e) {
                            console.warn('WebPersistence seed: failed to copy ' + src + ' -> ' + dst + ': ' + e.message);
                        }
                    } else {
                        skipped++;
                    }
                }
            }
        }
        console.log('WebPersistence: seeding /data from /data_readonly ...');
        seedDir('/data_readonly', '/data');
        console.log('WebPersistence: seed done — copied=' + copied + ' skipped=' + skipped);
        // Verify config.def is accessible
        try {
            FS.stat('/data/config.def');
            console.log('WebPersistence: /data/config.def is present');
        } catch(e) {
            console.error('WebPersistence: /data/config.def NOT found after seed!');
        }
    });
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool WebPersistence_Init()
{
    ensure_data_dir();

    if (!mount_idbfs()) {
        // IDBFS unavailable — fall back to plain MEMFS: seed game assets
        // directly so ReadConfig can still open data/config.def.  Saves
        // will not persist across reloads, but the game will at least run.
        fprintf(stderr, "WebPersistence_Init: IDBFS mount failed, running without persistence\n");
        seed_from_readonly();
        return true;
    }

    // Populate the in-memory FS from IndexedDB synchronously (Asyncify).
    // populate=true means "pull from IDB into memory".
    int ok = idbfs_syncfs_blocking(1);

    if (!ok) {
        // syncfs failed — still try seeding so the game can run.
        fprintf(stderr, "WebPersistence_Init: syncfs(populate) failed, seeding from readonly\n");
        seed_from_readonly();
        return true;
    }

    // Seed default assets into /data for any file not yet in IDBFS.
    seed_from_readonly();

    // Flush the seed writes to IndexedDB (blocking, so files are visible
    // to ReadConfig immediately after WebPersistence_Init returns).
    idbfs_syncfs_blocking(0);

    return true;
}

void WebPersistence_SyncAsync()
{
    // populate=false: flush in-memory changes to IndexedDB.
    EM_ASM({
        FS.syncfs(false, function(err) {
            if (err) console.error('WebPersistence: syncfs flush failed: ' + err);
        });
    });
}

void WebPersistence_SyncBlocking()
{
    // Best-effort: issue async flush (Asyncify not guaranteed here).
    WebPersistence_SyncAsync();
}

// ---------------------------------------------------------------------------
// Export / Import
// ---------------------------------------------------------------------------

// Export implementation: enumerate /data, encode each file as base64 in JS,
// build a simple JSON manifest, and trigger a browser download.
//
// Note: Full ZIP creation would require an additional library (e.g. JSZip).
// This first implementation downloads a JSON manifest with base64-encoded
// file contents.  See docs/EmscriptenToolchain.md for upgrade path.

extern "C" void FreeEggbert_ExportPersistentData()
{
    EM_ASM({
        // Recursively collect all files under /data.
        function collectFiles(dir) {
            var result = [];
            var entries;
            try { entries = FS.readdir(dir); } catch(e) { return result; }
            for (var i = 0; i < entries.length; i++) {
                var name = entries[i];
                if (name === '.' || name === '..') continue;
                var fullPath = dir + '/' + name;
                var stat;
                try { stat = FS.stat(fullPath); } catch(e) { continue; }
                if (FS.isDir(stat.mode)) {
                    var sub = collectFiles(fullPath);
                    result = result.concat(sub);
                } else {
                    try {
                        var data = FS.readFile(fullPath);
                        var binary = '';
                        for (var j = 0; j < data.length; j++) {
                            binary += String.fromCharCode(data[j]);
                        }
                        var b64 = btoa(binary);
                        // Store relative path (strip leading '/data/')
                        var relPath = fullPath.substring('/data/'.length);
                        result.push({ path: relPath, data: b64 });
                    } catch(e) {
                        console.warn('Export: skipping ' + fullPath + ': ' + e.message);
                    }
                }
            }
            return result;
        }

        var files = collectFiles('/data');
        if (files.length === 0) {
            console.log('WebPersistence export: /data is empty, nothing to export.');
            return;
        }

        var json = JSON.stringify({ version: 1, files: files }, null, 2);
        var blob = new Blob([json], { type: 'application/json' });
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href = url;
        a.download = 'free-eggbert-save.json';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
        console.log('WebPersistence: exported ' + files.length + ' file(s) to free-eggbert-save.json');
    });
}

extern "C" void FreeEggbert_ImportPersistentData()
{
    EM_ASM({
        var input = document.createElement('input');
        input.type = 'file';
        input.accept = '.json';
        input.onchange = function(e) {
            var file = e.target.files[0];
            if (!file) return;
            var reader = new FileReader();
            reader.onload = function(ev) {
                try {
                    var manifest = JSON.parse(ev.target.result);
                    if (!manifest.files || !Array.isArray(manifest.files)) {
                        console.error('WebPersistence import: invalid manifest format');
                        return;
                    }
                    var count = 0;
                    for (var i = 0; i < manifest.files.length; i++) {
                        var entry = manifest.files[i];
                        var relPath = entry.path;
                        // Security: reject absolute paths and path traversal.
                        if (!relPath || relPath.indexOf('..') !== -1 || relPath[0] === '/') {
                            console.warn('WebPersistence import: rejected unsafe path: ' + relPath);
                            continue;
                        }
                        var fullPath = '/data/' + relPath;
                        // Ensure parent directory exists.
                        var parts = fullPath.split('/');
                        var dir = '';
                        for (var j = 1; j < parts.length - 1; j++) {
                            dir += '/' + parts[j];
                            try { FS.mkdir(dir); } catch(ex) {}
                        }
                        // Decode and write the file.
                        try {
                            var binary = atob(entry.data);
                            var buf = new Uint8Array(binary.length);
                            for (var k = 0; k < binary.length; k++) buf[k] = binary.charCodeAt(k);
                            FS.writeFile(fullPath, buf);
                            count++;
                        } catch(ex) {
                            console.warn('WebPersistence import: failed to write ' + fullPath + ': ' + ex.message);
                        }
                    }
                    // Flush to IndexedDB.
                    FS.syncfs(false, function(err) {
                        if (err) console.error('WebPersistence import: syncfs failed: ' + err);
                        else console.log('WebPersistence: imported ' + count + ' file(s). Reload the page to apply.');
                    });
                } catch(ex) {
                    console.error('WebPersistence import: parse error: ' + ex.message);
                }
            };
            reader.readAsText(file);
        };
        document.body.appendChild(input);
        input.click();
        document.body.removeChild(input);
    });
}

#endif // __EMSCRIPTEN__
