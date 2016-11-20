cmake_minimum_required(VERSION 2.8.12)
project(chroma-emu)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g -Wall -Wextra -Wshadow")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
