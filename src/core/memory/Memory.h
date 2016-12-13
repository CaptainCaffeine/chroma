// This file is a part of Chroma.
// Copyright (C) 2016 Matthew Murray
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

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"

namespace Core {

struct CartridgeHeader;
class Timer;
class Serial;
class LCD;

class Memory {
public:
    Memory(const Console gb_type, const CartridgeHeader& header, Timer& tima, Serial& sio, LCD& display,
           std::vector<u8> rom_contents);

    const Console console;
    const GameMode game_mode;
    bool cgb_double_speed = false;

    u8 ReadMem8(const u16 addr) const;
    void WriteMem8(const u16 addr, const u8 data);

    // Interrupt functions
    void RequestInterrupt(Interrupt intr) {
        if (!IF_written_this_cycle) { interrupt_flags |= static_cast<unsigned int>(intr); }
    }
    void ClearInterrupt(Interrupt intr) {
        if (!IF_written_this_cycle) { interrupt_flags &= ~static_cast<unsigned int>(intr); }
    }
    bool IsPending(Interrupt intr) const { return interrupt_flags & hram.back() & static_cast<unsigned int>(intr); }
    bool RequestedEnabledInterrupts() const { return interrupt_flags & hram.back(); }
    bool IF_written_this_cycle = false;

    // DMA functions
    void UpdateOAM_DMA();

    // LCD functions
    template<typename DestIter>
    void CopyFromVRAM(const u16 start_addr, const std::size_t num_bytes, DestIter dest) const {
        std::copy_n(vram.cbegin() + (start_addr - 0x8000), num_bytes, dest);
    }

    template<typename DestIter>
    void CopyOAM(DestIter dest) const {
        std::copy(oam.cbegin(), oam.cend(), dest);
    }
private:
    Timer& timer;
    Serial& serial;
    LCD& lcd;

    const MBC mbc_mode;
    const bool ext_ram_present;
    const bool rumble_present;
    const int num_rom_banks;

    const std::vector<u8> rom;
    std::vector<u8> vram;
    std::vector<u8> wram;
    std::vector<u8> oam;
    std::vector<u8> hram;
    std::vector<u8> ext_ram;

    // I/O register functions
    void IORegisterInit();
    u8 ReadIORegisters(const u16 addr) const;
    void WriteIORegisters(const u16 addr, const u8 data);

    // DMA utilities
    enum class DMAState {Inactive, RegWritten, Starting, Active};
    DMAState state_oam_dma = DMAState::Inactive;
    bool dma_blocking_memory = false;

    u16 oam_transfer_addr;
    u8 oam_transfer_byte;
    unsigned int bytes_read = 0;

    u8 DMACopy(const u16 addr) const;

    // MBC functions
    u8 ReadExternalRAM(const u16 addr) const;
    void WriteExternalRAM(const u16 addr, const u8 data);
    void WriteMBCControlRegisters(const u16 addr, const u8 data);

    // ******** I/O registers ******** 
    // P1 register: 0xFF00
    //     bit 0: P10 Input Right or A (0=Pressed)
    //     bit 1: P11 Input Left or B (0=Pressed)
    //     bit 2: P12 Input Up or Select (0=Pressed)
    //     bit 3: P13 Input Down or Start (0=Pressed)
    //     bit 4: P14 Select Direction Keys (0=Select)
    //     bit 5: P15 Select Button Keys (0=Select)
    u8 joypad; // Not implemented.

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
    //     bit 0: VBLANK
    //     bit 1: LCD Status
    //     bit 2: Timer Overflow
    //     bit 3: Serial Transfer Complete
    //     bit 4: Joypad (high-to-low of I/O regs P10-P13)
    u8 interrupt_flags = 0x01;

    // NR10 register: 0xFF10
    u8 sweep_mode1 = 0x00;
    // NR11 register: 0xFF11
    u8 pattern_duty_mode1 = 0x80;
    // NR12 register: 0xFF12
    u8 envelope_mode1 = 0xF3;
    // NR13 register: 0xFF13
    u8 frequency_lo_mode1 = 0xFF;
    // NR14 register: 0xFF14
    u8 frequency_hi_mode1 = 0x00;
    // NR21 register: 0xFF16
    u8 pattern_duty_mode2 = 0x00;
    // NR22 register: 0xFF17
    u8 envelope_mode2 = 0x00;
    // NR23 register: 0xFF18
    u8 frequency_lo_mode2 = 0xFF;
    // NR24 register: 0xFF19
    u8 frequency_hi_mode2 = 0x00;
    // NR30 register: 0xFF1A
    u8 sound_on_mode3 = 0x00;
    // NR31 register: 0xFF1B
    u8 sound_length_mode3 = 0xFF;
    // NR32 register: 0xFF1C
    u8 output_mode3 = 0x00;
    // NR33 register: 0xFF1D
    u8 frequency_lo_mode3 = 0xFF;
    // NR34 register: 0xFF1E
    u8 frequency_hi_mode3 = 0x00;
    // NR41 register: 0xFF20
    u8 sound_length_mode4 = 0x1F;
    // NR42 register: 0xFF21
    u8 envelope_mode4 = 0x00;
    // NR43 register: 0xFF22
    u8 poly_counter_mode4 = 0x00;
    // NR44 register: 0xFF23
    u8 counter_mode4 = 0x00;
    // NR50 register: 0xFF24
    u8 volume = 0x77;
    // NR51 register: 0xFF25
    u8 sound_select = 0xF3;
    // NR52 register: 0xFF26
    u8 sound_on = 0x81;
    // Wave Pattern RAM: 0xFF30-0xFF3F
    std::array<u8, 0x10> wave_ram{};

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
    //     bits 7-6: background colour 3
    //     bits 5-4: background colour 2
    //     bits 3-2: background colour 1
    //     bits 1-0: background colour 0
    // OBP0 register: 0xFF48
    // OBP1 register: 0xFF49
    // WY register: 0xFF4A
    // WX register: 0xFF4B
    // Implementations located in LCD class.

    // DMA register: 0xFF46
    u8 oam_dma_start = 0xFF;

    // KEY1 register: 0xFF4D
    u8 speed_switch = 0x00;

    // HDMA1 register: 0xFF51
    u8 hdma_source_hi = 0x00; // Not implemented. What is the initial value of this register, if it has one?
    // HDMA2 register: 0xFF52
    u8 hdma_source_lo = 0x00; // Not implemented. What is the initial value of this register, if it has one?
    // HDMA3 register: 0xFF53
    u8 hdma_dest_hi = 0x00; // Not implemented. What is the initial value of this register, if it has one?
    // HDMA4 register: 0xFF54
    u8 hdma_dest_lo = 0x00; // Not implemented. What is the initial value of this register, if it has one?
    // HDMA5 register: 0xFF55
    u8 hdma_control = 0x00; // Not implemented. What is the initial value of this register?

    // VBK register: 0xFF4F
    unsigned int vram_bank_num = 0x00;
    // SVBK register: 0xFF70
    unsigned int wram_bank_num = 0x00;

    // ******** MBC control registers ******** 
    int rom_bank_num = 0x01;
    int ram_bank_num = 0x00;
    bool ext_ram_enabled = false;

    // MBC1
    bool ram_bank_mode = false;

    // MBC3
    unsigned int rtc_seconds = 0x00;
    unsigned int rtc_minutes = 0x00;
    unsigned int rtc_hours = 0x00;
    u8 rtc_day = 0x00;

    // bit 0: MSB(it) of Day Counter
    // bit 6: Halt (0=Active, 1=Stop Timer)
    // bit 7: Day Counter Carry Bit
    u8 rtc_flags = 0x00;
};

} // End namespace Core
