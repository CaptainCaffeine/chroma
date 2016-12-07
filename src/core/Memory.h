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
#include <algorithm>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"
#include "core/CartridgeHeader.h"

namespace Core {

class Memory {
public:
    Memory(const Console game_boy, const CartridgeHeader header, std::vector<u8> rom_contents);

    const Console console;
    const GameMode game_mode;
    bool cgb_double_speed = false;

    u8 ReadMem8(const u16 addr) const;
    u16 ReadMem16(const u16 addr) const;
    void WriteMem8(const u16 addr, const u8 data);
    void WriteMem16(const u16 addr, const u16 data);

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

    // Timer functions
    u16 ReadDIV() const { return divider; }
    void IncrementDIV(unsigned int cycles) { divider += cycles; }

    // LCD functions
    template<typename DestIter>
    void CopyFromVRAM(const u16 start_addr, const std::size_t num_bytes, DestIter dest) const {
        std::copy_n(vram.cbegin() + (start_addr - 0x8000), num_bytes, dest);
    }

    // ******** Temporarily public for LCD access.

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
    u8 lcdc = 0x91; // TODO: Verify that 0x91 is the correct initial value for this register.
    // STAT register: 0xFF41
    //     bit 6: LY=LYC Check Enable
    //     bit 5: Mode 2 OAM Check Enable
    //     bit 4: Mode 1 VBLANK Check Enable
    //     bit 3: Mode 0 HBLANK Check Enable
    //     bit 2: LY=LYC Compare Signal (1 implies LY=LYC)
    //     bits 1&0: Screen Mode (0=HBLANK, 1=VBLANK, 2=Searching OAM, 3=Transferring Data to LCD driver)
    u8 stat = 0x01; // What is the initial value of this register?
    // SCY register: 0xFF42
    u8 scroll_y = 0x00; // What is the initial value of this register?
    // SCX register: 0xFF43
    u8 scroll_x = 0x00; // What is the initial value of this register?
    // LY register: 0xFF44
    u8 ly = 0x00;
    // LYC register: 0xFF45
    u8 ly_compare = 0x00; // What is the initial value of this register?

    // DMA register: 0xFF46
    u8 oam_dma = 0x00; // Not implemented. What is the initial value of this register, if it has one?

    // BGP register: 0xFF47
    //     bits 7-6: background colour 3
    //     bits 5-4: background colour 2
    //     bits 3-2: background colour 1
    //     bits 1-0: background colour 0
    u8 bg_palette = 0x00; // What is the initial value of this register?
    // OBP0 register: 0xFF48
    u8 obj_palette0 = 0x00; // Not implemented. What is the initial value of this register?
    // OBP1 register: 0xFF49
    u8 obj_palette1 = 0x00; // Not implemented. What is the initial value of this register?
    // WY register: 0xFF4A
    u8 window_y = 0x00; // Not implemented. What is the initial value of this register?
    // WX register: 0xFF4B
    u8 window_x = 0x00; // Not implemented. What is the initial value of this register?

private:
    const MBC mbc_mode;
    const bool ext_ram_present;
    const unsigned int num_rom_banks;

    const std::vector<u8> rom;
    std::vector<u8> vram;
    std::vector<u8> wram;
    std::vector<u8> oam;
    std::vector<u8> hram;
    std::vector<u8> ext_ram;

    void IORegisterInit();
    u8 ReadIORegisters(const u16 addr) const;
    void WriteIORegisters(const u16 addr, const u8 data);
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
    u8 serial_data = 0x00; // Not implemented. What is the initial value of this register?
    // SC register: 0xFF02
    //     bit 7: Transfer Start Flag
    //     bit 1: Speed? CGB only?
    //     bit 0: Shift Clock (0=External Clock, 1=Internal Clock 8192Hz)
    u8 serial_control = 0x00; // Not implemented. What is the initial value of this register?

    // DIV register: 0xFF04
    u16 divider;
    // TIMA register: 0xFF05
    u8 timer_counter = 0x00;
    // TMA register: 0xFF06
    u8 timer_modulo = 0x00;
    // TAC register: 0xFF07
    //     bit 2: Timer Enable
    //     bits 1&0: Main Frequency Divider (0=every 1024 cycles, 1=16 cycles, 2=64 cycles, 3=256 cycles)
    u8 timer_control = 0x00;

    // IF register: 0xFF0F
    //     bit 0: VBLANK
    //     bit 1: LCD Status
    //     bit 2: Timer Overflow
    //     bit 3: Serial Transfer Complete
    //     bit 4: Joypad (high-to-low of I/O regs P10-P13)
    u8 interrupt_flags = 0x00;

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
    unsigned int rom_bank_num = 0x01;
    unsigned int ram_bank_num = 0x00;
    bool ext_ram_enabled = false;
    bool ram_bank_mode = false;
};

} // End namespace Core
