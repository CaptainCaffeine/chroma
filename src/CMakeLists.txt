# Specify includes relative to the src directory
include_directories(.)

set(COMMON_SOURCES
    common/Util.cpp
    common/logging/Disassembler.cpp
    common/logging/Logging.cpp)

set(COMMON_HEADERS
    common/CommonTypes.h
    common/CommonEnums.h
    common/Util.h
    common/logging/Logging.h)

set(CORE_SOURCES
    core/GameBoy.cpp
    core/cpu/CPU.cpp
    core/cpu/Ops.cpp
    core/Audio.cpp
    core/Channel.cpp
    core/Joypad.cpp
    core/Serial.cpp
    core/Timer.cpp
    core/lcd/LCD.cpp
    core/lcd/Debug.cpp
    core/memory/Memory.cpp
    core/memory/DMA.cpp
    core/memory/MBC.cpp
    core/memory/RTC.cpp
    core/CartridgeHeader.cpp)

set(CORE_HEADERS
    core/GameBoy.h
    core/cpu/CPU.h
    core/cpu/Flags.h
    core/Audio.h
    core/Channel.h
    core/Joypad.h
    core/Serial.h
    core/Timer.h
    core/lcd/LCD.h
    core/memory/Memory.h
    core/memory/RTC.h
    core/CartridgeHeader.h)

set(EMU_SOURCES
    emu/main.cpp
    emu/SDLContext.cpp
    emu/ParseOptions.cpp)

set(EMU_HEADERS
    emu/SDLContext.h
    emu/ParseOptions.h)

add_executable(chroma ${EMU_SOURCES}
                      ${EMU_HEADERS}
                      ${CORE_SOURCES}
                      ${CORE_HEADERS}
                      ${COMMON_SOURCES}
                      ${COMMON_HEADERS})

target_link_libraries(chroma ${SDL2_LIBRARY})
