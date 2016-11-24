# Specify includes relative to the src directory
include_directories(.)

set(COMMON_SOURCES common/CommonTypes.h common/CommonEnums.h)
set(CORE_SOURCES core/CPU.h core/CPU.cpp core/Ops.cpp core/Logging.cpp core/Serial.cpp core/Disassembler.h core/Disassembler.cpp core/Flags.h core/Timer.h core/Timer.cpp core/Memory.h core/Memory.cpp core/CartridgeHeader.h core/CartridgeHeader.cpp)
set(EMU_SOURCES emu/chroma.cpp emu/ParseOptions.h emu/ParseOptions.cpp)

add_executable(chroma ${EMU_SOURCES} ${CORE_SOURCES} ${COMMON_SOURCES})

target_link_libraries(chroma ${SDL2_LIBRARY})
