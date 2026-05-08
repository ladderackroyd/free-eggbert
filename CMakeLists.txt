cmake_minimum_required(VERSION 3.16)

project(SPEEDY_BLUPI_WINDOWS LANGUAGES CXX)

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
    add_subdirectory(../free-api FREE_API)
    add_subdirectory(../free-direct FREE_DIRECT)

    target_link_libraries(SPEEDY_BLUPI_WINDOWS
            PRIVATE
            free-api
            free-direct
    )

    free_eggbert_copy_sdl_runtime(SPEEDY_BLUPI_WINDOWS)

else()
    message(FATAL_ERROR "Unknown SPEEDY_BLUPI_BACKEND: ${SPEEDY_BLUPI_BACKEND}")
endif()