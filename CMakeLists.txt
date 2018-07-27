cmake_minimum_required(VERSION 3.1)
project(chroma-emu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

add_subdirectory(externals)
add_subdirectory(src)

target_compile_options(chroma PRIVATE -Wall -Wextra -Wshadow -fdiagnostics-color -msse2)
