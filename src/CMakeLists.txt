# Specify includes relative to the src directory
include_directories(.)

set(COMMON_HEADERS
    common/CommonTypes.h
    common/CommonEnums.h)

set(CORE_SOURCES
    core/GameBoy.cpp
    core/cpu/CPU.cpp
    core/cpu/Ops.cpp
    core/Disassembler.cpp
    core/Logging.cpp
    core/Serial.cpp
    core/Timer.cpp
    core/LCD.cpp
    core/memory/Memory.cpp
    core/memory/DMA.cpp
    core/memory/MBC.cpp
    core/CartridgeHeader.cpp)

set(CORE_HEADERS
    core/GameBoy.h
    core/cpu/CPU.h
    core/cpu/Flags.h
    core/Disassembler.h
    core/Serial.h
    core/Timer.h
    core/LCD.h
    core/memory/Memory.h
    core/CartridgeHeader.h)

set(EMU_SOURCES
    emu/chroma.cpp
    emu/SDL_Utils.cpp
    emu/ParseOptions.cpp)

set(EMU_HEADERS
    emu/SDL_Utils.h
    emu/ParseOptions.h)

add_executable(chroma ${EMU_SOURCES} ${EMU_HEADERS} ${CORE_SOURCES} ${CORE_HEADERS} ${COMMON_HEADERS})

target_link_libraries(chroma ${SDL2_LIBRARY})
