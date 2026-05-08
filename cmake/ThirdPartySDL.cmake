include_guard(GLOBAL)

option(FREE_USE_SYSTEM_SDL "Use system SDL packages instead of vendored submodules" OFF)

function(configure_vendored_sdl)
    if(TARGET SDL3::SDL3 AND TARGET SDL3_image::SDL3_image AND TARGET SDL3_mixer::SDL3_mixer)
        return()
    endif()

    if(FREE_USE_SYSTEM_SDL)
        find_package(SDL3 REQUIRED)
        find_package(SDL3_image REQUIRED)
        find_package(SDL3_mixer REQUIRED)
        return()
    endif()

    set(_third_party_root "${CMAKE_SOURCE_DIR}/third_party")
    foreach(_dep IN ITEMS SDL SDL_image SDL_mixer)
        if(NOT EXISTS "${_third_party_root}/${_dep}/CMakeLists.txt")
            message(FATAL_ERROR
                "Missing vendored dependency '${_dep}' in ${_third_party_root}. "
                "Run: git submodule update --init --recursive")
        endif()
    endforeach()

    set(SDL_SHARED ON CACHE BOOL "Build SDL as shared" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "Build SDL as static" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "Build SDL tests" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "Build SDL examples" FORCE)
    set(SDL_DISABLE_INSTALL ON CACHE BOOL "Disable SDL install target in superbuild" FORCE)

    set(SDLIMAGE_VENDORED ON CACHE BOOL "Use vendored dependencies in SDL_image" FORCE)
    set(SDLIMAGE_TESTS OFF CACHE BOOL "Build SDL_image tests" FORCE)
    set(SDLIMAGE_SAMPLES OFF CACHE BOOL "Build SDL_image samples" FORCE)
    set(SDLIMAGE_INSTALL OFF CACHE BOOL "Disable SDL_image install target in superbuild" FORCE)
    set(SDLIMAGE_AVIF OFF CACHE BOOL "Disable AVIF support" FORCE)
    set(SDLIMAGE_JXL OFF CACHE BOOL "Disable JXL support" FORCE)
    set(SDLIMAGE_TIF OFF CACHE BOOL "Disable TIFF support" FORCE)
    set(SDLIMAGE_WEBP OFF CACHE BOOL "Disable WEBP support" FORCE)
    set(SDLIMAGE_PNG_LIBPNG OFF CACHE BOOL "Use internal PNG loader instead of libpng" FORCE)

    set(SDLMIXER_VENDORED ON CACHE BOOL "Use vendored dependencies in SDL_mixer" FORCE)
    set(SDLMIXER_TESTS OFF CACHE BOOL "Build SDL_mixer tests" FORCE)
    set(SDLMIXER_EXAMPLES OFF CACHE BOOL "Build SDL_mixer examples" FORCE)
    set(SDLMIXER_INSTALL OFF CACHE BOOL "Disable SDL_mixer install target in superbuild" FORCE)
    set(SDLMIXER_GME OFF CACHE BOOL "Disable GME support" FORCE)
    set(SDLMIXER_MOD_XMP OFF CACHE BOOL "Disable libxmp backend" FORCE)
    set(SDLMIXER_MP3_MPG123 OFF CACHE BOOL "Disable mpg123 backend" FORCE)
    set(SDLMIXER_MIDI_FLUIDSYNTH OFF CACHE BOOL "Disable FluidSynth backend" FORCE)
    set(SDLMIXER_OPUS OFF CACHE BOOL "Disable Opus support" FORCE)
    set(SDLMIXER_VORBIS_VORBISFILE OFF CACHE BOOL "Disable libvorbisfile backend" FORCE)
    set(SDLMIXER_VORBIS_TREMOR OFF CACHE BOOL "Disable tremor backend" FORCE)
    set(SDLMIXER_WAVPACK OFF CACHE BOOL "Disable WavPack support" FORCE)
    set(SDLMIXER_FLAC_LIBFLAC OFF CACHE BOOL "Disable libFLAC backend" FORCE)

    add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/SDL EXCLUDE_FROM_ALL)
    add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/SDL_image EXCLUDE_FROM_ALL)
    add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/SDL_mixer EXCLUDE_FROM_ALL)
endfunction()

function(free_eggbert_copy_sdl_runtime target_name)
    if(NOT WIN32)
        return()
    endif()

    foreach(_dep_target IN ITEMS SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer)
        if(TARGET ${_dep_target})
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${_dep_target}>
                    $<TARGET_FILE_DIR:${target_name}>
                VERBATIM)
        endif()
    endforeach()
endfunction()

function(free_eggbert_copy_mingw_runtime target_name)
    if(NOT WIN32 OR NOT MINGW)
        return()
    endif()

    # Locate the MinGW bin directory from the CXX compiler path
    get_filename_component(_mingw_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)

    set(_mingw_dlls
        "libgcc_s_seh-1.dll"
        "libstdc++-6.dll"
        "libwinpthread-1.dll"
    )

    foreach(_dll IN LISTS _mingw_dlls)
        set(_dll_path "${_mingw_bin_dir}/${_dll}")
        if(EXISTS "${_dll_path}")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_dll_path}"
                    "$<TARGET_FILE_DIR:${target_name}>"
                COMMENT "Deploying MinGW runtime: ${_dll}"
                VERBATIM)
        endif()
    endforeach()
endfunction()