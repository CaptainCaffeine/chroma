# Specify includes relative to the src directory
include_directories(.)

set(SOURCES
    gb/core/GameBoy.cpp
    gb/cpu/Cpu.cpp
    gb/cpu/Ops.cpp
    gb/audio/Audio.cpp
    gb/audio/Channel.cpp
    gb/hardware/Joypad.cpp
    gb/hardware/Serial.cpp
    gb/hardware/Timer.cpp
    gb/lcd/Lcd.cpp
    gb/lcd/Debug.cpp
    gb/memory/Memory.cpp
    gb/memory/Dma.cpp
    gb/memory/Mbc.cpp
    gb/memory/Rtc.cpp
    gb/memory/CartridgeHeader.cpp
    gb/logging/Disassembler.cpp
    gb/logging/Logging.cpp

    gba/core/Core.cpp
    gba/memory/Memory.cpp
    gba/memory/CartridgeHeader.cpp
    gba/memory/Save.cpp
    gba/cpu/Cpu.cpp
    gba/cpu/Instruction.cpp
    gba/cpu/ArmOps.cpp
    gba/cpu/ThumbOps.cpp
    gba/cpu/Disassembler.cpp
    gba/cpu/ArmDisasm.cpp
    gba/cpu/ThumbDisasm.cpp
    gba/lcd/Lcd.cpp
    gba/lcd/Bg.cpp
    gba/lcd/Debug.cpp
    gba/audio/Audio.cpp
    gba/hardware/Timer.cpp
    gba/hardware/Dma.cpp
    gba/hardware/Keypad.cpp

    common/Screenshot.cpp

    emu/main.cpp
    emu/SdlContext.cpp
    emu/ParseOptions.cpp
   )

set(HEADERS
    gb/core/GameBoy.h
    gb/core/Enums.h
    gb/cpu/Cpu.h
    gb/audio/Audio.h
    gb/audio/Channel.h
    gb/hardware/Joypad.h
    gb/hardware/Serial.h
    gb/hardware/Timer.h
    gb/lcd/Lcd.h
    gb/memory/Memory.h
    gb/memory/Rtc.h
    gb/memory/CartridgeHeader.h
    gb/logging/Logging.h

    gba/core/Core.h
    gba/memory/Memory.h
    gba/memory/MemDefs.h
    gba/memory/IOReg.h
    gba/cpu/Cpu.h
    gba/cpu/CpuDefs.h
    gba/cpu/Instruction.h
    gba/cpu/Disassembler.h
    gba/lcd/Lcd.h
    gba/lcd/Bg.h
    gba/audio/Audio.h
    gba/hardware/Timer.h
    gba/hardware/Dma.h
    gba/hardware/Keypad.h
    gba/hardware/Serial.h

    common/CommonTypes.h
    common/CommonFuncs.h
    common/CommonEnums.h
    common/Screenshot.h
    common/RingBuffer.h
    common/Biquad.h
    common/Vec4f.h

    emu/SdlContext.h
    emu/ParseOptions.h
   )

add_executable(chroma ${SOURCES} ${HEADERS})

target_link_libraries(chroma PRIVATE ${SDL2_LIBRARY} fmt::fmt PNG::PNG)
