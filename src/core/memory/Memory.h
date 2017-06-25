// This file is a part of Chroma.
// Copyright (C) 2016-2017 Matthew Murray
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <vector>
#include <array>
#include <algorithm>
#include <memory>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"

// Forward declaration for cross-namespace friend declaration.
namespace Log { class Logging; }

namespace Core {

class CartridgeHeader;
class Timer;
class Serial;
class LCD;
class Joypad;
class Audio;
class RTC;

class Memory {
    friend class Log::Logging;
public:
    Memory(const Console gb_type, const CartridgeHeader& header, Timer& tima, Serial& sio, LCD& display,
           Joypad& pad, Audio& audio, const std::vector<u8>& rom_contents, std::vector<u8>& save_game);
    ~Memory();

    const Console console;
    const GameMode game_mode;
    unsigned int double_speed = 0;

    u8 ReadMem8(const u16 addr) const;
    void WriteMem8(const u16 addr, const u8 data);

    void ToggleCPUSpeed() {
        speed_switch = (speed_switch ^ 0x80) & 0x80;
        double_speed ^= 1;
    };

    // Interrupt functions
    void RequestInterrupt(Interrupt intr) {
        if (!IF_written_this_cycle) { interrupt_flags |= static_cast<unsigned int>(intr); }
    }
    void ClearInterrupt(Interrupt intr) {
        if (!IF_written_this_cycle) { interrupt_flags &= ~static_cast<unsigned int>(intr); }
    }
    bool IsPending(Interrupt intr) const {
        return interrupt_flags & interrupt_enable & static_cast<unsigned int>(intr);
    }
    bool RequestedEnabledInterrupts() const { return interrupt_flags & interrupt_enable; }
    bool IF_written_this_cycle = false;

    // DMA functions
    void UpdateOAM_DMA();
    void UpdateHDMA();
    bool HDMAInProgress() const { return hdma_state == DMAState::Active || hdma_state == DMAState::Starting; }
    void SignalHDMA();

    // LCD functions
    template<typename DestIter>
    void CopyFromVRAM(const u16 start_addr, const std::size_t num_bytes, const int bank_num, DestIter dest) const {
        std::copy_n(vram.cbegin() + (start_addr - 0x8000) + 0x2000 * bank_num, num_bytes, dest);
    }

    // MBC/Saving functions
    void SaveExternalRAM(const std::string& save_path) const;
private:
    Timer& timer;
    Serial& serial;
    LCD& lcd;
    Joypad& joypad;
    Audio& audio;

    const MBC mbc_mode;
    const bool ext_ram_present;
    const bool rtc_present;
    const bool rumble_present;
    const int num_rom_banks;
    const int num_ram_banks;

    const std::vector<u8>& rom;
    std::vector<u8> vram;
    std::vector<u8> wram;
    std::vector<u8> hram;
    std::vector<u8>& ext_ram;
    std::unique_ptr<RTC> rtc;

    // Init functions
    void IORegisterInit();
    void VRAMInit();

    // I/O register functions
    u8 ReadIORegisters(const u16 addr) const;
    void WriteIORegisters(const u16 addr, const u8 data);

    // DMA utilities
    enum class DMAState {Inactive, Starting, Active, Paused};
    enum class Bus {None, External, VRAM};
    DMAState oam_dma_state = DMAState::Inactive;
    Bus dma_bus_block = Bus::None;

    u16 oam_transfer_addr;
    u8 oam_transfer_byte;
    unsigned int bytes_read = 160;

    enum class HDMAType {GDMA, HDMA};
    DMAState hdma_state = DMAState::Inactive;
    HDMAType hdma_type;
    bool hdma_reg_written = false;
    int bytes_to_copy = 0, hblank_bytes = 0;

    void InitHDMA();
    void ExecuteHDMA();
    u8 DMACopy(const u16 addr) const;

    // MBC functions
    u8 ReadExternalRAM(const u16 addr) const;
    void WriteExternalRAM(const u16 addr, const u8 data);
    void WriteMBCControlRegisters(const u16 addr, const u8 data);

    // ******** I/O registers ******** 
    // P1 register: 0xFF00
    //     bit 5: P15 Select Button Keys (0=Select)
    //     bit 4: P14 Select Direction Keys (0=Select)
    //     bit 3: P13 Input Down or Start (0=Pressed)
    //     bit 2: P12 Input Up or Select (0=Pressed)
    //     bit 1: P11 Input Left or B (0=Pressed)
    //     bit 0: P10 Input Right or A (0=Pressed)
    // Implementation located in Joypad class.

    // SB register: 0xFF01
    // SC register: 0xFF02
    //     bit 7: Transfer Start Flag
    //     bit 1: Transfer Speed (0=Normal, 1=Fast) (CGB Only)
    //     bit 0: Shift Clock (0=External Clock, 1=Internal Clock 8192Hz)
    // Implementations located in Serial class.

    // DIV register: 0xFF04
    // TIMA register: 0xFF05
    // TMA register: 0xFF06
    // TAC register: 0xFF07
    //     bit 2: Timer Enable
    //     bits 1&0: Main Frequency Divider (0=every 1024 cycles, 1=16 cycles, 2=64 cycles, 3=256 cycles)
    // Implementations located in Timer class.

    // IF register: 0xFF0F
    //     bit 4: Joypad
    //     bit 3: Serial Transfer Complete
    //     bit 2: Timer Overflow
    //     bit 1: LCD Status
    //     bit 0: VBLANK
    u8 interrupt_flags = 0x01;

    // NR10 register: 0xFF10
    //     bit 6-4: Sweep Time (n/128Hz)
    //     bit 3:   Sweep Direction (0=increase, 1=decrease)
    //     bit 2-0: Sweep Shift
    // NR11 register: 0xFF11
    //     bit 7-6: Wave Pattern Duty
    //     bit 5-0: Sound Length (Write Only)
    // NR12 register: 0xFF12
    //     bit 7-4: Initial Volume of Envelope
    //     bit 3:   Envelope Direction (0=decrease, 1=increase)
    //     bit 2-0: Step Time (n/64 seconds)
    // NR13 register: 0xFF13 (Write Only)
    // NR14 register: 0xFF14
    //     bit 7:   Reset (1=Restart Sound) (Write Only)
    //     bit 6:   Timed Mode (0=continuous, 1=stop when length expires)
    //     bit 2-0: Frequency High 3 Bits (Write Only)
    // NR21 register: 0xFF16
    //     bit 7-6: Wave Pattern Duty
    //     bit 5-0: Sound Length (Write Only)
    // NR22 register: 0xFF17
    //     bit 7-4: Initial Volume of Envelope
    //     bit 3:   Envelope Direction (0=decrease, 1=increase)
    //     bit 2-0: Step Time (n/64 seconds)
    // NR23 register: 0xFF18
    // NR24 register: 0xFF19
    //     bit 7:   Reset (1=Restart Sound) (Write Only)
    //     bit 6:   Timed Mode (0=continuous, 1=stop when length expires)
    //     bit 2-0: Frequency High 3 Bits (Write Only)
    // NR30 register: 0xFF1A
    //     bit 7: Channel On/Off (0=Off, 1=On)
    // NR31 register: 0xFF1B
    //     bit 7-0: Sound Length
    // NR32 register: 0xFF1C
    //     bit 6-5: Output Level (0=Silent, 1=unshifted/100%, 2=shifted right once/50%, 3=shifted right twice/25%)
    // NR33 register: 0xFF1D
    // NR34 register: 0xFF1E
    //     bit 7:   Reset (1=Restart Sound) (Write Only)
    //     bit 6:   Timed Mode (0=continuous, 1=stop when length expires)
    //     bit 2-0: Frequency High 3 Bits (Write Only)
    // NR41 register: 0xFF20
    //     bit 5-0: Sound Length
    // NR42 register: 0xFF21
    //     bit 7-4: Initial Volume of Envelope
    //     bit 3:   Envelope Direction (0=decrease, 1=increase)
    //     bit 2-0: Step Time (n/64 seconds)
    // NR43 register: 0xFF22
    //     bit 7-4: Shift Clock Frequency
    //     bit 3:   Counter Step (0=15 bits, 1=7 bits)
    //     bit 2-0: Clock Divider
    // NR44 register: 0xFF23
    //     bit 7: Reset (1=Restart Sound) (Write Only)
    //     bit 6: Timed Mode (0=continuous, 1=stop when length expires)
    // NR50 register: 0xFF24
    //     bit 7:   Output Vin to SO2 (1=Enable)
    //     bit 6-4: SO2 Output Level
    //     bit 3:   Output Vin to SO1 (1=Enable)
    //     bit 2-0: SO1 Output Level
    // NR51 register: 0xFF25
    //     bit 7: Output Channel 4 to SO2
    //     bit 6: Output Channel 3 to SO2
    //     bit 5: Output Channel 2 to SO2
    //     bit 4: Output Channel 1 to SO2
    //     bit 3: Output Channel 4 to SO1
    //     bit 2: Output Channel 3 to SO1
    //     bit 1: Output Channel 2 to SO1
    //     bit 0: Output Channel 1 to SO1
    // NR52 register: 0xFF26
    //     bit 7: Master Sound On/Off (0=Off)
    //     bit 3: Channel 4 On (Read Only)
    //     bit 2: Channel 3 On (Read Only)
    //     bit 1: Channel 2 On (Read Only)
    //     bit 0: Channel 1 On (Read Only)
    // Wave Pattern RAM: 0xFF30-0xFF3F
    // Implementations located in Audio class.

    // LCDC register: 0xFF40
    //     bit 7: LCD On
    //     bit 6: Window Tilemap Region (0=0x9800-0x9BFF, 1=0x9C00-0x9FFF)
    //     bit 5: Window Enable
    //     bit 4: BG and Window Tile Data Region (0=0x8800-0x97FF, 1=0x8000-0x8FFF)
    //     bit 3: BG Tilemap Region (0=0x9800-0x9BFF, 1=0x9C00-0x9FFF)
    //     bit 2: Sprite Size (0=8x8, 1=8x16)
    //     bit 1: Sprites Enabled
    //     bit 0: BG Enabled (0=On DMG, this sets the background to white.
    //                          On CGB in DMG mode, this disables both the window and background. 
    //                          In CGB mode, this gives all sprites priority over the background and window.)
    // STAT register: 0xFF41
    //     bit 6: LY=LYC Check Enable
    //     bit 5: Mode 2 OAM Check Enable
    //     bit 4: Mode 1 VBLANK Check Enable
    //     bit 3: Mode 0 HBLANK Check Enable
    //     bit 2: LY=LYC Compare Signal (1 implies LY=LYC)
    //     bits 1&0: Screen Mode (0=HBLANK, 1=VBLANK, 2=Searching OAM, 3=Transferring Data to LCD driver)
    // SCY register: 0xFF42
    // SCX register: 0xFF43
    // LY register: 0xFF44
    // LYC register: 0xFF45
    // BGP register: 0xFF47
    //     bits 7-6: Background Colour 3
    //     bits 5-4: Background Colour 2
    //     bits 3-2: Background Colour 1
    //     bits 1-0: Background Colour 0
    // OBP0 register: 0xFF48
    // OBP1 register: 0xFF49
    // WY register: 0xFF4A
    // WX register: 0xFF4B
    // BGPI register: 0xFF68
    //     bits 7: Auto Increment (1=Increment after write)
    //     bits 5-0: Palette Index (0x00-0x3F)
    // BGPD register: 0xFF69
    //     bits 7-0 in first byte, 8-14 in second byte
    //     bits 14-10: Blue Intensity
    //     bits 9-5:   Green Intensity
    //     bits 0-4:   Red Intensity
    // OBPI register: 0xFF6A
    //     bits 7: Auto Increment (1=Increment after write)
    //     bits 5-0: Palette Index (0x00-0x3F)
    // OBP1 register: 0xFF6B
    //     bits 7-0 in first byte, 8-14 in second byte
    //     bits 14-10: Blue Intensity
    //     bits 9-5:   Green Intensity
    //     bits 0-4:   Red Intensity
    // Implementations located in LCD class.

    // DMA register: 0xFF46
    u8 oam_dma_start;

    // KEY1 register: 0xFF4D
    u8 speed_switch = 0x00;

    // HDMA1 register: 0xFF51
    u8 hdma_source_hi = 0xFF;
    // HDMA2 register: 0xFF52
    u8 hdma_source_lo = 0xFF;
    // HDMA3 register: 0xFF53
    u8 hdma_dest_hi = 0xFF;
    // HDMA4 register: 0xFF54
    u8 hdma_dest_lo = 0xFF;
    // HDMA5 register: 0xFF55
    u8 hdma_control = 0xFF;

    // RP register: 0xFF56
    u8 infrared = 0x02; // Not implemented.

    // VBK register: 0xFF4F
    unsigned int vram_bank_num = 0x00;
    // SVBK register: 0xFF70
    unsigned int wram_bank_num = 0x00;

    // IE register: 0xFFFF
    //     bit 4: Joypad
    //     bit 3: Serial Transfer Complete
    //     bit 2: Timer Overflow
    //     bit 1: LCD Status
    //     bit 0: VBLANK
    u8 interrupt_enable = 0x00;

    // Undocumented CGB registers: 0xFF6C, 0xFF72, 0xFF73, 0xFF74, 0xFF75, 0xFF76, 0xFF77
    std::array<u8, 5> undocumented{{ 0x00, 0x00, 0x00, 0x00, 0x00 }};

    // ******** MBC control registers ******** 
    int rom_bank_num = 0x01;
    int ram_bank_num = 0x00;
    bool ext_ram_enabled = false;

    // MBC1
    int upper_bits = 0x00;
    bool ram_bank_mode = false;
};

} // End namespace Core
