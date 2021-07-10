cmake_minimum_required(VERSION 3.9)
project(chroma-emu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# External libraries
add_subdirectory(externals/fmt)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

find_package(ZLIB)
if (ZLIB_FOUND)
    message(STATUS "Using system zlib")
else()
    message(STATUS "Couldn't find system zlib, falling back on bundled library")
    add_subdirectory(externals/zlib)
    add_library(ZLIB::ZLIB ALIAS zlib)
endif()

find_package(PNG)
if (PNG_FOUND)
    message(STATUS "Using system libpng")
    include_directories(${PNG_INCLUDE_DIRS})
    add_definitions(${PNG_DEFINITIONS})
else()
    message(STATUS "Couldn't find system libpng, falling back on bundled library")
    add_subdirectory(externals/libpng)
    add_library(PNG::PNG ALIAS png_static)
endif()

add_subdirectory(src)

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)

if (lto_supported)
    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_property(TARGET chroma PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
else()
    message(STATUS "LTO not supported: ${error}")
endif()

target_compile_options(chroma PRIVATE -Wall -Wextra -Wshadow -fdiagnostics-color -msse2)
