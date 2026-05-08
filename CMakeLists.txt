cmake_minimum_required(VERSION 3.16)

project(SPEEDY_BLUPI_WINDOWS LANGUAGES CXX)

# -----------------------------------------------------------------------------
# Include-path hygiene rules for this build
# -----------------------------------------------------------------------------
# free-api ships compatibility headers that intentionally use legacy WinAPI/CRT
# names (windows.h, io.h, direct.h, ...). On MinGW, exposing such a directory
# globally (e.g. via include_directories()) silently shadows MinGW's own
# headers and breaks libstdc++ <cmath>/<cstdio> resolution while compiling
# bundled SDL.
#
# Therefore:
#   * DO NOT use bare include_directories(...) at this scope or above SDL.
#   * DO NOT prepend project root, src/, include/ or freeapi compat dirs to
#     CMAKE_CXX_FLAGS / CMAKE_C_FLAGS or to a global INTERFACE target.
#   * Use target_include_directories(<our-target> PRIVATE/PUBLIC ...) only.
#   * Compatibility headers are owned by the dedicated INTERFACE target
#     `freeapi_compat_headers` (defined in free-api/CMakeLists.txt) and are
#     reachable ONLY through `free-api` -> game/free-direct. SDL and other
#     third_party targets must never link against `freeapi_compat_headers`.
# -----------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/ThirdPartySDL.cmake)

# Backend selection:
#   FREEDIRECT - native DirectX 3 replacement backend
set(SPEEDY_BLUPI_BACKEND "FREEDIRECT" CACHE STRING "Backend: FREEDIRECT")
set_property(CACHE SPEEDY_BLUPI_BACKEND PROPERTY STRINGS FREEDIRECT)

# Setting the output file
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Adding source code files
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(SPEEDY_BLUPI_WINDOWS ${SOURCES})

# On Emscripten the output is HTML+JS+WASM; set the suffix accordingly.
if(EMSCRIPTEN)
    set_target_properties(SPEEDY_BLUPI_WINDOWS PROPERTIES SUFFIX ".html")
endif()

target_include_directories(SPEEDY_BLUPI_WINDOWS
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(SPEEDY_BLUPI_WINDOWS
            PRIVATE
            /permissive-
            /W0
    )
else()
    target_compile_options(SPEEDY_BLUPI_WINDOWS
            PRIVATE
            -fpermissive
            -fms-extensions
            -fPIC
            -w
            -Wno-narrowing
            -Wno-int-to-pointer-cast
            -g
            -O0
    )
endif()

if (SPEEDY_BLUPI_BACKEND STREQUAL "FREEDIRECT")
    message(STATUS "Using FREEDIRECT backend")

    target_compile_definitions(SPEEDY_BLUPI_WINDOWS PRIVATE FREEDIRECT)
    configure_vendored_sdl()

    # Diagnostics: enable internal counters & heartbeat in Free Direct (1 s snapshots).
    # Default ON in this build; set FREE_DIRECT_DIAGNOSTICS=OFF to disable. Counters
    # are no-ops at runtime unless env var FREE_DIRECT_DIAGNOSTICS=1 is also set.
    set(FREE_DIRECT_DIAGNOSTICS ON CACHE BOOL "Enable Free Direct diagnostics counters")

    # Disable free-api tests by default when building for Emscripten (they are
    # not runnable in the browser and would add unnecessary build overhead).
    if(EMSCRIPTEN)
        set(FREE_API_BUILD_TESTS OFF CACHE BOOL "Build free-api tests" FORCE)
    endif()

    add_subdirectory(../free-api FREE_API)
    add_subdirectory(../free-direct FREE_DIRECT)
    target_link_libraries(SPEEDY_BLUPI_WINDOWS
            PRIVATE
            free-api
            free-direct
    )

    # Emscripten-specific link options: produce HTML output and preload game data
    # directly from the repository asset directories into the virtual filesystem.
    if(EMSCRIPTEN)
        target_link_options(SPEEDY_BLUPI_WINDOWS PRIVATE
            -sUSE_SDL=0
            -sALLOW_MEMORY_GROWTH=1
            -sINITIAL_MEMORY=67108864
            "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/gamefiles/DATA@/data"
            "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/gamefiles/IMAGE08@/image08"
            "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/gamefiles/IMAGE16@/image16"
            "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/gamefiles/SOUND@/sound"
            -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']
        )
    endif()

    free_eggbert_copy_sdl_runtime(SPEEDY_BLUPI_WINDOWS)
    free_eggbert_copy_mingw_runtime(SPEEDY_BLUPI_WINDOWS)

else()
    message(FATAL_ERROR "Unknown SPEEDY_BLUPI_BACKEND: ${SPEEDY_BLUPI_BACKEND}")
endif()
