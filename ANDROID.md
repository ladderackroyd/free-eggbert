# Android Build Instructions — Free Eggbert

This document explains how to build, install, and run Free Eggbert as an
Android APK using SDL3, the Android NDK, and CMake.

---

## Prerequisites

| Tool | Recommended version |
|------|---------------------|
| Android Studio | Ladybug (2024.2) or newer |
| Android SDK | API level 35 |
| Android NDK | 30.0.14904198 (installed via SDK Manager) |
| CMake (NDK bundle) | 3.21+ (installed via SDK Manager) |
| Java (JDK) | 17 (bundled with Android Studio) |
| Git | any recent version |

### Install NDK and CMake via Android Studio

1. Open **Android Studio → Settings → SDK Manager → SDK Tools**.
2. Check **NDK (Side by side)** version **30.0.14904198**.
3. Check **CMake** (version 3.21 or higher).
4. Click **Apply** and let Android Studio download and install.

---

## Clone and initialise submodules

```bash
git clone <repository-url> free-eggbert
cd free-eggbert
git submodule update --init --recursive
```

The vendored SDL3 / SDL_image / SDL_mixer sources are required. They live under
`third_party/`.

---

## Game assets

Game assets are already linked via symbolic links in `android/app/src/main/assets/`:

```
android/app/src/main/assets/data     -> ../../../../../gamefiles/DATA
android/app/src/main/assets/image08  -> ../../../../../gamefiles/IMAGE08
android/app/src/main/assets/image16  -> ../../../../../gamefiles/IMAGE16
android/app/src/main/assets/sound    -> ../../../../../gamefiles/SOUND
```

If your build system does not follow symbolic links, copy the directories instead:

```bash
cd android/app/src/main/assets
rm -f data image08 image16 sound
cp -r ../../../../../gamefiles/DATA  data
cp -r ../../../../../gamefiles/IMAGE08 image08
cp -r ../../../../../gamefiles/IMAGE16 image16
cp -r ../../../../../gamefiles/SOUND  sound
```

The file `android/app/src/main/assets/freeapi_android_assets.txt` tells FreeApi
which directories to extract from the APK to internal storage at first launch.
Its contents must match the asset directories listed above:

```
data
image08
image16
sound
```

Do not remove or rename this file — FreeApi reads it at startup.

---

## Configure local.properties

Create the file `android/local.properties` (based on the provided template):

```bash
cp android/local.properties.template android/local.properties
# Edit the file and set sdk.dir to your Android SDK path, e.g.:
# sdk.dir=/home/user/Android/Sdk
```

---

## Build the debug APK

```bash
cd android
./gradlew assembleDebug
```

The first build downloads Gradle (≈ 150 MB) and compiles the NDK libraries, so
it may take several minutes. Subsequent incremental builds are much faster.

The produced APK is at:

```
android/app/build/outputs/apk/debug/app-debug.apk
```

---

## Install and run on a device or emulator

### Select an ADB device

If you have more than one device/emulator connected, set `ANDROID_SERIAL`:

```bash
export ANDROID_SERIAL=<device-serial>   # e.g. emulator-5554 or R3CN90ABCDE
adb devices                             # list available devices
```

### Fresh install

```bash
# Clear any previous install (resets extracted assets and save data)
adb shell pm clear org.openeggbert.freeeggbert

# Install the debug APK
adb install -r android/app/build/outputs/apk/debug/app-debug.apk

# Launch the game
adb shell am start -n org.openeggbert.freeeggbert/.FreeEggbertActivity
```

---

## Capture logcat

```bash
adb logcat | grep -E "SDL|FREEAPI|FREE_DIRECT|Eggbert|free-eggbert"
```

Key log tags to look for:

| Tag | Meaning |
|-----|---------|
| `free-api: SDL_main starting` | SDL_main entry point reached |
| `free-api: FreeApiAndroidSetup begin` | Android asset setup started |
| `free-api: internal storage path = …` | Internal storage path resolved |
| `free-api: extracting APK assets to …` | Asset extraction started |
| `FREEAPI_ANDROID: smoke-test data/config.def EXISTS` | Required config file found |
| `free-api: asset extraction complete` | All assets extracted successfully |
| `free-api: CWD set to …` | Working directory set to internal storage |
| `FREE_DIRECT_PRESENT: source=…` | Render loop is running (once per second) |
| `FREE_DIRECT_INPUT: raw=…` | Input events being processed |

---

## Build a release APK

### Create a signing keystore

```bash
keytool -genkey -v -keystore android/free-eggbert-release.keystore \
        -alias free-eggbert -keyalg RSA -keysize 2048 -validity 10000
```

### Create key.properties

Create `android/key.properties` (this file is gitignored — never commit it):

```
storeFile=../free-eggbert-release.keystore
storePassword=<your-store-password>
keyAlias=free-eggbert
keyPassword=<your-key-password>
```

### Build the signed release APK

```bash
cd android
./gradlew assembleRelease
```

The release APK is at:

```
android/app/build/outputs/apk/release/app-release.apk
```

---

## Asset layout in the APK

At runtime, FreeApi extracts APK assets to the app's internal storage directory
and sets the process working directory there. The game then accesses files via
normal relative `fopen()` paths:

| Game path | Source in APK assets |
|-----------|----------------------|
| `data/config.def` | `assets/data/config.def` |
| `data/world001.blp` | `assets/data/world001.blp` |
| `image08/…` | `assets/image08/…` |
| `image16/…` | `assets/image16/…` |
| `sound/…` | `assets/sound/…` |

The extraction is skipped on subsequent launches if the APK has not changed
(sentinel file `.freeapi_extracted` in internal storage tracks the APK timestamp).

---

## Rendering

Free Eggbert renders at its original fixed resolution. On widescreen Android
displays, FreeDirect uses SDL3's logical presentation with
`SDL_LOGICAL_PRESENTATION_LETTERBOX`, which automatically adds black bars on
the sides or top/bottom to preserve the original aspect ratio. The game image
is never stretched.

The present path logs once per second:

```
FREE_DIRECT_PRESENT: source/backbuffer=WxH window=WxH mode=letterbox …
```

If this log does not appear, the render loop has not started.

---

## Input

Touch and mouse coordinates are automatically mapped from physical screen
coordinates into game logical coordinates by SDL3's logical presentation layer.
Touches on black bars do not register as clicks inside the game.

Input diagnostic log (first 20 events):

```
FREE_DIRECT_INPUT: raw=x,y mapped=x,y evtType=… inside=true/false
```

---

## Troubleshooting

### `INSTALL_FAILED_UPDATE_INCOMPATIBLE`

The package signature changed. Uninstall the old version first:

```bash
adb uninstall org.openeggbert.freeeggbert
adb install android/app/build/outputs/apk/debug/app-debug.apk
```

### `adb: device not found`

- Enable **Developer Options** and **USB debugging** on your Android device.
- Try `adb devices` to confirm the device is listed.

### App exits immediately after "Running main function"

If logcat shows `Finished main function` almost immediately after
`Running main function SDL_main`, the game's `WinMain` is returning early.
The most common cause is that FreeApi's asset extraction failed and
`fopen("data/config.def")` returned NULL.

Check logcat for `free-api:` log lines:

```bash
adb logcat -s SDL
```

Look for:

- `free-api: freeapi_android_assets.txt not found` — the manifest is missing
  from the APK assets; verify `android/app/src/main/assets/freeapi_android_assets.txt` exists.
- `FREEAPI_ANDROID: smoke-test data/config.def MISSING` — assets were not
  extracted; check symlinks under `android/app/src/main/assets/`.
- `free-api: SDL_main exiting ret=0` — WinMain returned FALSE; check
  initialisation errors earlier in the log.

### Message loop exits immediately (WM_QUIT posted at startup)

```bash
adb logcat -s SDL | grep FREEAPI_ANDROID
```

Key tags:

| Log tag | Meaning |
|---------|---------|
| `FREEAPI_ANDROID: PostQuitMessage called` | `PostQuitMessage()` was called |
| `FREEAPI_ANDROID: SDL event SDL_EVENT_QUIT translated to WM_QUIT` | SDL sent a quit event |
| `FREEAPI_ANDROID: DefWindowProcA WM_CLOSE -> DestroyWindow + PostQuitMessage` | WM_CLOSE fell through to DefWindowProcA |

### App crashes on launch

Check `adb logcat` for native crash details. Common causes:

- Missing `libmain.so` — rebuild with `./gradlew assembleDebug`.
- Asset not found — verify that `data/`, `image08/`, `image16/`, and `sound/`
  exist under `android/app/src/main/assets/` and are correctly linked or copied.
- `git submodule update --init --recursive` not completed.

### Audio is silent

SDL_mixer must be built with WAV support (enabled by default in the vendored
build). If you see mixer errors in logcat, check that the sound files are
present in the APK:

```bash
aapt dump resources app/build/outputs/apk/debug/app-debug.apk | grep sound
```

### Desktop Linux build broken after these changes

The Android changes in `CMakeLists.txt` are fully conditional on `if(ANDROID)`.
Desktop builds are unaffected.
