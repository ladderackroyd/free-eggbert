cmake_minimum_required(VERSION 3.16)

project(SPEEDY_BLUPI_WINDOWS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Backend selection:
#   WINELIB    - build against WineLib / Wine DirectX libraries
#   FREEDIRECT - future native DirectX 3 replacement backend
set(SPEEDY_BLUPI_BACKEND "FREEDIRECT" CACHE STRING "Backend: WINELIB or FREEDIRECT")
set_property(CACHE SPEEDY_BLUPI_BACKEND PROPERTY STRINGS WINELIB FREEDIRECT)

# Setting the output file
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Adding source code files
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(SPEEDY_BLUPI_WINDOWS ${SOURCES})

target_include_directories(SPEEDY_BLUPI_WINDOWS
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(SPEEDY_BLUPI_WINDOWS
        PRIVATE
        -fpermissive
        -fPIC
        -w
        -g
        -O0
)

if(SPEEDY_BLUPI_BACKEND STREQUAL "WINELIB")
    message(STATUS "Using WINELIB backend")

    target_compile_definitions(SPEEDY_BLUPI_WINDOWS PRIVATE WINELIB)

    # Wine compiler setup should normally be passed before configure:
    #   cmake -B build -DCMAKE_CXX_COMPILER=wineg++-stable -DCMAKE_C_COMPILER=winegcc-stable
    #
    # Do not force CMAKE_CXX_COMPILER here after project(), because it may be too late.

    target_compile_options(SPEEDY_BLUPI_WINDOWS PRIVATE -m64)

    target_include_directories(SPEEDY_BLUPI_WINDOWS
            PRIVATE
            /usr/include/wine
    )

    target_link_directories(SPEEDY_BLUPI_WINDOWS
            PRIVATE
            /usr/lib/wine
    )

    #find_library(DPLAYX_LIBRARY NAMES dplayx PATHS /home/robertvokac/.wine/drive_c/windows/system32)

    target_link_libraries(SPEEDY_BLUPI_WINDOWS
            PRIVATE
            uuid
            user32
            gdi32
            ole32
            oleaut32
            shell32
            ddraw
            dsound

            # DirectPlay candidates / old Wine DLL experiments:
            # /home/robertvokac/.wine/drive_c/windows/system32/dplayx.dll
            # /home/robertvokac/.wine/drive_c/windows/system32/dpnet.dll
            # /home/robertvokac/.wine/drive_c/windows/system32/dpnlobby.dll
    )

elseif(SPEEDY_BLUPI_BACKEND STREQUAL "FREEDIRECT")
    message(STATUS "Using FREEDIRECT backend")

    target_compile_definitions(SPEEDY_BLUPI_WINDOWS PRIVATE FREEDIRECT)

    # TODO:
    # Future native backend replacing the DirectX 3 SDK / WineLib dependency.
    #
    # Expected future libraries may include:
    #   free-direct
    #   SDL3
    #   SDL3_mixer
    #
    # target_link_libraries(SPEEDY_BLUPI_WINDOWS PRIVATE free-direct)

else()
    message(FATAL_ERROR "Unknown SPEEDY_BLUPI_BACKEND: ${SPEEDY_BLUPI_BACKEND}")
endif()