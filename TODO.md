# TODO

This TODO reflects the current repository state. It is written as a development checklist for the repository itself, not as an analysis of any uploaded archive.

## Current build and dependency state

The current CMake configuration is focused on the native `FREEDIRECT` backend.

Current state:

- `SPEEDY_BLUPI_BACKEND` is restricted to `FREEDIRECT`.
- The game executable links to:
    - `free-api`
    - `free-direct`
- The game does not link SDL directly.
- Vendored SDL support exists through `cmake/ThirdPartySDL.cmake`.
- `.gitmodules` includes:
    - `dxsdk3`
    - `third_party/SDL`
    - `third_party/SDL_image`
    - `third_party/SDL_mixer`
- `FREE_USE_SYSTEM_SDL` defaults to `OFF`, so SDL should be built from vendored submodules by default.
- System SDL should only be used when explicitly requested with `-DFREE_USE_SYSTEM_SDL=ON`.

TODO:

- [ ] Verify that a clean checkout builds after running:

```bash
git submodule update --init --recursive
cmake -S . -B build -DSPEEDY_BLUPI_BACKEND=FREEDIRECT
cmake --build build -j
```

- [ ] Verify that the default Linux build does not require system-installed SDL packages.
- [ ] Verify that `FREE_USE_SYSTEM_SDL=ON` still works as an optional developer override.
- [ ] Verify that `cmake/ThirdPartySDL.cmake` only defines/configures vendored SDL and does not leak SDL into the game target.
- [ ] Verify that `free-api` and `free-direct` link SDL dependencies as `PRIVATE`, not `PUBLIC`.
- [ ] Verify that the game target does not receive SDL include directories transitively.
- [ ] Verify that no game source file contains direct SDL usage.
- [ ] Verify that public Free API / Free Direct headers do not expose SDL types.

## CMake cleanup and portability

Current CMake state is already much better than an earlier system-SDL setup, but there are still cleanup points.

### Executable include visibility

The game executable currently uses:

```cmake
target_include_directories(SPEEDY_BLUPI_WINDOWS
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

Executable targets normally do not need public include directories.

TODO:

- [ ] Consider changing this to `PRIVATE`:

```cmake
target_include_directories(SPEEDY_BLUPI_WINDOWS
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

- [ ] Ensure this does not affect the current build.

### Compiler options

Current CMake has separate MSVC and non-MSVC compile options.

Non-MSVC options include:

```cmake
-fpermissive
-fms-extensions
-fPIC
-w
-Wno-narrowing
-Wno-int-to-pointer-cast
-g
-O0
```

MSVC options currently include:

```cmake
/permissive-
/W0
```

TODO:

- [ ] Verify whether `/permissive-` is appropriate for this decompiled code.
- [ ] If MSVC fails due to old/decompiled C++ constructs, consider whether `/permissive-` should be removed.
- [ ] Keep compiler-option changes separate from gameplay/decompilation fixes.
- [ ] Avoid changing warning policy unless needed for a specific compiler/build failure.
- [ ] Ensure Linux, Windows/MSVC, and Windows/MinGW or Clang builds are handled intentionally.

### Vendored SDL CMake helper

`cmake/ThirdPartySDL.cmake` defines:

```cmake
function(configure_vendored_sdl)
```

and the main `CMakeLists.txt` calls:

```cmake
configure_vendored_sdl()
```

This is currently consistent.

TODO:

- [ ] Keep the function name consistent.
- [ ] Do not reintroduce old names such as `free_eggbert_configure_sdl_dependencies()` unless a compatibility wrapper is intentionally needed.
- [ ] Consider whether `CMAKE_SOURCE_DIR` should be replaced with a more local path such as `PROJECT_SOURCE_DIR` or a path based on `CMAKE_CURRENT_LIST_DIR`, especially if this project is ever used as a subproject.
- [ ] Verify that the SDL submodule directories contain actual checked-out sources after `git submodule update --init --recursive`.

### SDL runtime copying on Windows

`free_eggbert_copy_sdl_runtime(SPEEDY_BLUPI_WINDOWS)` copies SDL runtime libraries on Windows.

TODO:

- [ ] Verify that `$<TARGET_FILE:SDL3::SDL3>` works for the actual SDL target exported by vendored SDL.
- [ ] Verify the same for `SDL3_image::SDL3_image` and `SDL3_mixer::SDL3_mixer`.
- [ ] If alias targets do not work with `$<TARGET_FILE:...>`, use the concrete SDL shared-library targets exposed by SDL.
- [ ] Ensure runtime copying does not add SDL as a direct game dependency.
- [ ] Ensure the executable can run from the build directory on Windows after copying DLLs.

## Dependency boundary: game vs Free API / Free Direct vs SDL

The intended architecture is:

```text
Free Eggbert game code
        ↓
Free API / Free Direct public compatibility headers
        ↓
Free API / Free Direct implementation
        ↓
SDL / SDL_image / SDL_mixer internals
```

The game is allowed to include WinAPI-like and DirectX-like compatibility headers such as:

```text
windows.h
windowsx.h
wtypes.h
mmsystem.h
digitalv.h
ddraw.h
dsound.h
dplay.h
direct.h
io.h
```

The game must not include SDL directly.

TODO:

- [ ] Keep WinAPI-like / DirectX-like includes in the game source acceptable.
- [ ] Do not force the game to include SDL.
- [ ] Do not force the game to link SDL directly.
- [ ] Do not add Free API headers as source files to the game executable.
- [ ] Keep SDL private to Free API / Free Direct.
- [ ] Keep public Free API / Free Direct APIs WinAPI-like / DirectX-like, not SDL-like.
- [ ] Scan game sources for accidental SDL usage:

```bash
grep -R "#include <SDL\|#include \"SDL\|SDL_" -n src include
```

- [ ] Scan CMake for accidental direct SDL linkage from the game target.

## Investigate DirectDraw fullscreen initialization and decompilation artifacts

The game currently runs in fullscreen mode on Free Direct / Free API, but the same fullscreen path may fail when built against the original DirectX 3 SDK.

This suggests that Free Direct may currently be more tolerant than real DirectDraw, or that the decompiled fullscreen initialization path is not fully correct.

The main suspected issue is that `CPixmap::Create()` may be called twice during startup without clearly releasing the previously created DirectDraw objects.

### Suspected double `CPixmap::Create()` call

Startup currently initializes the pixmap like this:

```cpp
g_pPixmap = new CPixmap;

if (!g_pPixmap->Create(g_hWnd, totalDim, g_bFullScreen,
                       g_mouseType, g_bTrueColorBack, g_bTrueColorDecor))
    return InitFail("Create pixmap", TRUE);

if (!g_pPixmap->CacheAll(TRUE, g_hWnd, g_bFullScreen,
                         g_bTrueColorBack, g_bTrueColorDecor,
                         g_mouseType, "init.blp", 0))
    return FALSE;
```

However, `CPixmap::CacheAll()` currently calls `Create()` again:

```cpp
if (Create(hWnd, dim, bFullScreen, mouseType, bTrueColorDecor, bTrueColor) == FALSE)
{
    return FALSE;
}
```

This may cause startup to do the following:

```text
new CPixmap
  -> CPixmap::Create()
       -> DirectDrawCreate
       -> SetCooperativeLevel
       -> SetDisplayMode
       -> CreateSurface primary
       -> CreateSurface back
       -> CreateSurface mouse

  -> CPixmap::CacheAll(TRUE, ...)
       -> CPixmap::Create() again
            -> DirectDrawCreate again
            -> SetCooperativeLevel again
            -> SetDisplayMode again
            -> CreateSurface primary again
```

Real DirectDraw may reject the second initialization attempt with errors such as:

```text
DDERR_PRIMARYSURFACEALREADYEXISTS
DDERR_EXCLUSIVEMODEALREADYSET
DDERR_NOEXCLUSIVEMODE
DDERR_INVALIDPARAMS
DDERR_WRONGMODE
```

Free Direct may tolerate this if it does not strictly enforce all original DirectDraw lifetime and exclusive-mode rules.

TODO:

- [ ] Confirm whether `CPixmap::Create()` is really called twice during startup.
- [ ] Add logging at the start and end of `CPixmap::Create()`.
- [ ] Log whether `m_lpDD`, `m_lpDDSPrimary`, `m_lpDDSBack`, and `m_lpDDSMouse` already exist before creating new DirectDraw objects.
- [ ] Log all DirectDraw `HRESULT` values during both the first and second initialization attempts.
- [ ] Determine whether the double `Create()` call is intentional or a decompilation artifact.
- [ ] Decide which function owns DirectDraw initialization:
    - either `DoInit()` creates the pixmap,
    - or `CacheAll()` creates/recreates the pixmap,
    - but both should not create DirectDraw objects without a clear release/reinitialization path.
- [ ] If recreation is intentional, add or verify a correct cleanup path before the second initialization.
- [ ] Do not silently make Free Direct more tolerant until the real DirectDraw behavior is understood.

### Suspicious `bTrueColorBack` / `bTrueColorDecor` parameter mismatch

In `include/pixmap.hpp`, the declaration is:

```cpp
BOOL Create(HWND hwnd, POINT dim, BOOL bFullScreen, int mouseType,
            BOOL bTrueColor, BOOL bTrueColorDecor);
```

This suggests the intended order is probably:

```text
bTrueColorBack
bTrueColorDecor
```

However, the definition in `src/pixmap.cpp` is currently:

```cpp
BOOL CPixmap::Create(HWND hwnd, POINT dim,
                     BOOL bFullScreen, int mouseType,
                     BOOL bTrueColorDecor, BOOL bTrueColor)
```

The parameter names are swapped.

Then the function assigns:

```cpp
m_bTrueColorDecor = bTrueColorDecor;
m_bTrueColorBack = bTrueColor;
```

In `blupi.cpp`, the call is:

```cpp
Create(..., g_bTrueColorBack, g_bTrueColorDecor)
```

But the definition interprets this as:

```text
g_bTrueColorBack  -> decor
g_bTrueColorDecor -> back
```

Inside `CacheAll()`, the call appears to compensate by using the opposite order:

```cpp
Create(hWnd, dim, bFullScreen, mouseType, bTrueColorDecor, bTrueColor)
```

This may work accidentally when both values are the same, but may break when they differ.

TODO:

- [ ] Audit the intended meaning of `bTrueColorBack` and `bTrueColorDecor`.
- [ ] Compare declaration, definition, and all call sites of `CPixmap::Create()`.
- [ ] Determine whether the parameter order is a decompilation artifact.
- [ ] Rename parameters locally for clarity once the intended order is confirmed.
- [ ] Avoid changing behavior until logging proves the current behavior is wrong.
- [ ] Test configurations where background and decor truecolor settings differ.

### Dangerous `CacheAll(FALSE)` code path

`CPixmap::CacheAll(FALSE)` currently contains code similar to:

```cpp
if (cache == FALSE)
{
    delete this;
    hWnd = m_hWnd;
    bFullScreen = m_bFullScreen;
    bTrueColor = m_bTrueColorBack;
    bTrueColorDecor = m_bTrueColorDecor;
    mouseType = m_mouseType;
}
```

This is unsafe.

After:

```cpp
delete this;
```

the object is destroyed, but the code then continues to read members such as:

```text
m_hWnd
m_bFullScreen
m_bTrueColorBack
m_bTrueColorDecor
m_mouseType
```

Reading object members after `delete this` is undefined behavior.

TODO:

- [ ] Investigate whether this code path is actually executed.
- [ ] Treat this as a likely decompilation artifact or incorrectly reconstructed logic.
- [ ] Reconstruct the intended original behavior.
- [ ] Never access object members after `delete this`.
- [ ] If object recreation is required, save needed values before destruction.
- [ ] Prefer explicit cleanup/reinitialize methods over `delete this`.

### Suspicious rectangle initialization in `CacheAll(TRUE)`

There is suspicious decompiled code similar to:

```cpp
char image[12];

*(char*)image = (LXIMAGE) << 64;
rect.bottom = LYIMAGE;
rect.left = LOWORD(image);
rect.top = HIWORD(image);
rect.right = HIWORD(image);
DrawImage(0, 0, rect, 1);
Display();
```

This does not make sense.

Problems:

```text
LXIMAGE is likely 640, so shifting it by 64 is nonsensical.
image is a local char array.
LOWORD(image) / HIWORD(image) on a pointer or array is suspicious.
rect is probably initialized incorrectly.
```

The original code may have been something like:

```cpp
SetRect(&rect, 0, 0, LXIMAGE, LYIMAGE);
```

or another normal rectangle initialization.

TODO:

- [ ] Investigate this code in Ghidra.
- [ ] Compare with Planet Blupi source if a similar initialization exists.
- [ ] Replace this only after confirming the intended behavior.
- [ ] Verify what rectangle should be passed to `DrawImage()`.
- [ ] Add comments once the original intent is understood.
- [ ] Do not leave pointer-based `LOWORD` / `HIWORD` artifacts if they are proven invalid.

### DirectDraw fullscreen setup

`CPixmap::Create()` currently uses this DirectDraw setup:

```cpp
DirectDrawCreate(NULL, &m_lpDD, NULL);

if (m_bFullScreen)
    SetCooperativeLevel(hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
else
    SetCooperativeLevel(hwnd, DDSCL_NORMAL);

if (m_bFullScreen)
    SetDisplayMode(dim.x, dim.y, colorMode);

CreateSurface(primary);
CreateSurface(system-memory backbuffer);
CreateSurface(system-memory mousebuffer);
```

This is not a classic fullscreen flip-chain setup.

It does not appear to use:

```text
DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX
DDSD_BACKBUFFERCOUNT
GetAttachedSurface
Flip
```

However, this may still be valid if the game draws into a system-memory backbuffer and then blits to the primary surface.

Likely intended model:

```text
draw everything into m_lpDDSBack
then Blt from m_lpDDSBack to the primary surface
```

Real DirectDraw fullscreen mode is stricter about:

```text
display mode
color depth
primary surface caps
exclusive mode
palette state
surface lifetime
```

Free Direct may be simpler because it can internally do something like:

```text
SDL fullscreen window
software/system-memory surface
copy/blit to renderer
```

TODO:

- [ ] Confirm whether the game uses `Blt` rather than `Flip`.
- [ ] Confirm whether the original game used a primary-surface + system-memory-backbuffer model.
- [ ] Verify whether this model works with real DirectDraw 3 in fullscreen.
- [ ] Log `SetDisplayMode()` parameters and return values.
- [ ] Log primary and backbuffer `DDSURFACEDESC` values.
- [ ] Verify palette behavior in 8-bit mode.
- [ ] Verify 16-bit / truecolor behavior.
- [ ] Do not assume the lack of a flip chain is a bug until tested against original behavior.

### Required DirectDraw diagnostics

Add detailed logging around DirectDraw initialization calls:

- [ ] `DirectDrawCreate`
- [ ] `SetCooperativeLevel`
- [ ] `SetDisplayMode`
- [ ] `CreateSurface` for the primary surface
- [ ] `CreateSurface` for the back buffer
- [ ] `CreateSurface` for the mouse surface
- [ ] `CreateClipper`
- [ ] `SetPalette`
- [ ] `Blt`
- [ ] `Flip`, if ever used

Log the following values:

- [ ] fullscreen/windowed mode
- [ ] requested width and height
- [ ] requested color depth
- [ ] `DDSURFACEDESC.dwSize`
- [ ] `DDSURFACEDESC.dwFlags`
- [ ] `DDSCAPS.dwCaps`
- [ ] returned `HRESULT`
- [ ] whether DirectDraw objects already exist before initialization

Minimum useful diagnostic patch:

```text
CPixmap::Create enter
CPixmap::Create leave
m_lpDD before creation
m_lpDDSPrimary before creation
m_lpDDSBack before creation
m_lpDDSMouse before creation
bFullScreen
bTrueColorBack / bTrueColorDecor
requested width / height
requested color depth
DirectDrawCreate HRESULT
SetCooperativeLevel HRESULT
SetDisplayMode HRESULT
CreateSurface primary HRESULT
CreateSurface back HRESULT
CreateSurface mouse HRESULT
```

## Classic Win32 message loop and future web support

`WinMain()` starts a multimedia timer:

```cpp
g_updateTimer = timeSetEvent(g_timerInterval, g_timerInterval / 4,
                             TimerStep, NULL, TIME_PERIODIC);
```

Then it runs a classic Win32 message loop:

```cpp
while (TRUE)
{
    if (PeekMessage(&msg, NULL, 0,0, PM_NOREMOVE))
    {
        if (!GetMessage(&msg, NULL, 0, 0))
            return msg.wParam;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    else
    {
        if (!g_bActive) WaitMessage();
    }
}
```

The timer callback posts update messages, and the window procedure handles `WM_UPDATE` roughly by doing:

```cpp
UpdateFrame();
SetDecor();
g_pPixmap->Display();
```

This is important for future web support.

Emscripten does not work well with a traditional blocking infinite message loop. The web version will likely need a Free API / launcher-level main-loop adapter rather than direct changes to the game logic.

TODO:

- [ ] Keep the native Win32-style loop intact for desktop builds.
- [ ] For web builds, design a Free API or launcher-level adapter.
- [ ] Do not hack `src/blupi.cpp` just to satisfy Emscripten.
- [ ] Identify the minimum frame function needed for web:
    - event pump
    - `WM_UPDATE`
    - `UpdateFrame()`
    - `SetDecor()`
    - `Display()`
- [ ] Investigate whether `WinMain()` can be adapted without modifying game logic.
- [ ] Keep web work separate from DirectDraw fullscreen debugging.

## Audio backend state

Current state:

```cpp
#define _BASS FALSE
```

With `_BASS` set to `FALSE`, the active implementation is probably the old DirectSound / MCI-style path in `soundold.cpp`:

```cpp
#if !_BASS || _LEGACY
...
#endif
```

`sound.cpp` uses the opposite condition:

```cpp
#if _BASS && !_LEGACY
...
#endif
```

Both files are present in the project, but preprocessor conditions should prevent conflicting definitions.

This works, but the setup is fragile. The README still mentions BASS / BASSMIDI, so the documentation and current configuration should be verified.

TODO:

- [ ] Document the currently active audio backend.
- [ ] Verify whether `_BASS FALSE` is intentional.
- [ ] Verify whether README audio notes are outdated or still relevant.
- [ ] Do not mix audio backend cleanup with DirectDraw fullscreen investigation.
- [ ] Later, consider a CMake option such as:

```cmake
FREE_EGGBERT_AUDIO_BACKEND=DSOUND|BASS|STUB
```

- [ ] Keep audio backend selection separate from SDL vendoring and fullscreen debugging.

## Network / DirectPlay state

`CNetwork::CreateProvider(0)` is called during startup:

```cpp
g_pNetwork = new CNetwork;
g_pNetwork->CreateProvider(0);
```

However, `CreateProvider()` returns `FALSE` immediately if:

```cpp
if (index >= m_providers.nb) return FALSE;
```

Since `m_providers.nb` starts at 0, and no required `EnumProviders()` call is visible before this startup call, network initialization likely fails silently during normal startup.

The return value appears to be ignored, so the game continues in singleplayer.

There is also suspicious receive logic: `Receive()` reads into a local `dataBuffer`, zeroes `pDest`, but does not appear to copy the received data into `pDest` before returning `TRUE`.

TODO:

- [ ] Treat networking / DirectPlay as incomplete.
- [ ] Verify whether singleplayer intentionally ignores failed network setup.
- [ ] Audit `CNetwork::EnumProviders()`.
- [ ] Audit `CNetwork::CreateProvider()`.
- [ ] Audit `CNetwork::Receive()` and whether it should copy data into the caller buffer.
- [ ] Do not let DirectPlay work block DirectDraw fullscreen or rendering work.
- [ ] Consider stubbing network cleanly for singleplayer builds.

## DirectX SDK / dxsdk3 submodule status

Current `.gitmodules` still contains:

```text
[submodule "dxsdk3"]
    path = dxsdk3
    url = https://github.com/jummy0/dxsdk3.git
```

Current CMake is focused on `FREEDIRECT`, but the repository still contains legacy Visual Studio / DirectX-related project files and the `dxsdk3` submodule reference.

TODO:

- [ ] Decide whether `dxsdk3` remains required for reference, legacy builds, or real DirectX 3 SDK comparison builds.
- [ ] If `dxsdk3` is only historical/reference material, document that clearly.
- [ ] If real DirectX 3 SDK comparison builds are still needed, add explicit build instructions.
- [ ] Keep DirectX 3 SDK comparison work separate from Free Direct portability work.

## Debugging priority

Do not try to solve SDL vendoring, audio, web support, CMake cleanup, DirectPlay, and DirectDraw fullscreen at the same time.

Recommended priority order:

1. Add detailed logging around DirectDraw initialization.
2. Verify whether `CPixmap::Create()` is really called twice.
3. Capture the exact `HRESULT` from the second `DirectDrawCreate`, `SetCooperativeLevel`, `SetDisplayMode`, and `CreateSurface` calls.
4. Decide ownership of `CPixmap::Create()`:
    - either `DoInit()` creates it,
    - or `CacheAll()` creates it,
    - but not both without a clear cleanup path.
5. Clarify the `bTrueColorBack` / `bTrueColorDecor` parameter order.
6. Keep unrelated systems out of this investigation.
7. Only after the fullscreen path is understood, continue with web/Android portability work.

## Main conclusion

There are at least three strong reasons why fullscreen can behave differently on Free Direct than on the original DirectX 3 SDK:

1. `CPixmap::Create()` appears to be called twice during startup without releasing previous DirectDraw objects.
2. `bTrueColorBack` / `bTrueColorDecor` are inconsistent between declaration, definition, and call sites.
3. `CacheAll()` contains likely decompilation artifacts such as:
    - `delete this` followed by further member access,
    - suspicious `LOWORD` / `HIWORD` use on local data,
    - nonsensical rectangle initialization.

Therefore, this is not only a question of Free Direct being tolerant. There are concrete suspicious areas in the decompiled game code that real DirectX 3 SDK may expose as errors.

TODO:

- [ ] Treat the current fullscreen mismatch as a game reconstruction / DirectDraw contract investigation.
- [ ] Do not assume Free Direct is wrong just because it runs.
- [ ] Do not assume the decompiled game code is correct just because Free Direct tolerates it.
- [ ] Use DirectX 3 SDK errors as useful feedback about invalid or incomplete reconstruction.
- [ ] Eventually consider adding a strict compatibility mode to Free Direct:

```text
FREE_DIRECT_STRICT=1
```

Strict mode could warn or fail when the game uses DirectDraw in a way that the original DirectX 3 SDK would likely reject.

Goal:

Make the decompiled fullscreen path valid enough to work both on Free Direct / Free API and the original DirectX 3 SDK, or clearly document why Free Direct intentionally accepts behavior that real DirectDraw rejects.