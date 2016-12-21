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

#include "core/CartridgeHeader.h"
#include "core/memory/Memory.h"
#include "core/Timer.h"
#include "core/Serial.h"
#include "core/LCD.h"
#include "core/Joypad.h"

namespace Core {

Memory::Memory(const Console gb_type, const CartridgeHeader& header, Timer& tima, Serial& sio, LCD& display,
               Joypad& pad, std::vector<u8> rom_contents)
        : console(gb_type)
        , game_mode(header.game_mode)
        , timer(tima)
        , serial(sio)
        , lcd(display)
        , joypad(pad)
        , mbc_mode(header.mbc_mode)
        , ext_ram_present(header.ext_ram_present)
        , rumble_present(header.rumble_present)
        , num_rom_banks(header.num_rom_banks)
        , rom(std::move(rom_contents)) {

    if (game_mode == GameMode::DMG) {
        // 8KB VRAM and WRAM
        vram = std::vector<u8>(0x2000);
        wram = std::vector<u8>(0x2000);
    } else if (game_mode == GameMode::CGB) {
        // 16KB VRAM and 32KB WRAM
        vram = std::vector<u8>(0x4000);
        wram = std::vector<u8>(0x8000);
    }

    if (ext_ram_present) {
        ext_ram = std::vector<u8>(header.ram_size);
    }

    // 160 bytes object attribute memory.
    oam = std::vector<u8>(0xA0);
    // 127 bytes high RAM.
    hram = std::vector<u8>(0x7F);

    IORegisterInit();
}

void Memory::IORegisterInit() {
    if (game_mode == GameMode::DMG) {
        if (console == Console::DMG) {
            joypad.p1 = 0xCF; // DMG starts with joypad inputs enabled.
            timer.divider = 0xABCC;
        } else {
            joypad.p1 = 0xFF; // CGB starts with joypad inputs disabled, even in DMG mode.
            timer.divider = 0x267C;
        }
    } else {
        joypad.p1 = 0xFF; // Probably?
        timer.divider = 0x1EA0;
    }
}

u8 Memory::ReadMem8(const u16 addr) const {
    if (addr >= 0xFF00) {
        // 0xFF00-0xFFFF are still accessible during OAM DMA.
        if (addr < 0xFF80) {
            // I/O registers
            return ReadIORegisters(addr);
        } else if (addr < 0xFFFF) {
            // High RAM
            return hram[addr - 0xFF80];
        } else {
            // Interrupt enable (IE) register
            return interrupt_enable;
        }
    } else if (!dma_blocking_memory) {
        if (addr < 0x4000) {
            // Fixed ROM bank
            return rom[addr];
        } else if (addr < 0x8000) {
            // Switchable ROM bank.
            return rom[addr + 0x4000*((rom_bank_num % num_rom_banks) - 1)];
        } else if (addr < 0xA000) {
            // VRAM -- switchable in CGB mode
            // Not accessible during screen mode 3.
            if ((lcd.stat & 0x03) != 3) {
                return vram[addr - 0x8000 + 0x2000*vram_bank_num];
            } else {
                return 0xFF;
            }
        } else if (addr < 0xC000) {
            // External RAM bank.
            return ReadExternalRAM(addr);
        } else if (addr < 0xD000) {
            // WRAM bank 0
            return wram[addr - 0xC000];
        } else if (addr < 0xE000) {
            // WRAM bank 1 (switchable from 1-7 in CGB mode)
            return wram[addr - 0xC000 + 0x1000*((wram_bank_num == 0) ? 0 : wram_bank_num-1)];
        } else if (addr < 0xFE00) {
            // Echo of C000-DDFF
            return wram[addr - 0xE000 + 0x1000*((wram_bank_num == 0) ? 0 : wram_bank_num-1)];
            // For some unlicensed games and flashcarts on pre-CGB devices, reads from this region read both WRAM and
            // exernal RAM, and bitwise AND the two values together (source: AntonioND timing docs).
        } else if (addr < 0xFEA0) {
            // OAM (Sprite Attribute Table)
            // Not accessible during screen modes 2 or 3.
            if ((lcd.stat & 0x02) == 0) {
                return oam[addr - 0xFE00];
            } else {
                return 0xFF;
            }
        } else {
            // Unusable region
            // Pre-CGB devices: reads return 0x00
            // CGB: reads vary, refer to TCAGBD
            // AGB: reads return 0xNN where N is the high nybble of the lower byte of addr.
            return 0x00;
        }
    } else {
        return 0xFF;
    }
}

void Memory::WriteMem8(const u16 addr, const u8 data) {
    if (addr >= 0xFF00) {
        // 0xFF00-0xFFFF are still accessible during OAM DMA.
        if (addr < 0xFF80) {
            // I/O registers
            WriteIORegisters(addr, data);
        } else if (addr < 0xFFFF) {
            // High RAM
            hram[addr - 0xFF80] = data;
        } else {
            // Interrupt enable (IE) register
            interrupt_enable = data;
        }
    } else if (!dma_blocking_memory) {
        if (addr < 0x8000) {
            // MBC control registers -- writes to this region do not write the ROM.
            WriteMBCControlRegisters(addr, data);
        } else if (addr < 0xA000) {
            // VRAM -- switchable in CGB mode
            // Not accessible during screen mode 3.
            if ((lcd.stat & 0x03) != 3) {
                vram[addr - 0x8000 + 0x2000*vram_bank_num] = data;
            }
        } else if (addr < 0xC000) {
            // External RAM bank.
            WriteExternalRAM(addr, data);
        } else if (addr < 0xD000) {
            // WRAM bank 0
            wram[addr - 0xC000] = data;
        } else if (addr < 0xE000) {
            // WRAM bank 1 (switchable from 1-7 in CGB mode)
            wram[addr - 0xC000 + 0x1000*((wram_bank_num == 0) ? 0 : wram_bank_num-1)] = data;
        } else if (addr < 0xFE00) {
            // Echo of C000-DDFF
            wram[addr - 0xE000 + 0x1000*((wram_bank_num == 0) ? 0 : wram_bank_num-1)] = data;
            // For some unlicensed games and flashcarts on pre-CGB devices, writes to this region write to both WRAM 
            // and external RAM (source: AntonioND timing docs).
        } else if (addr < 0xFEA0) {
            // OAM (Sprite Attribute Table)
            // Not accessible during screen modes 2 or 3.
            if ((lcd.stat & 0x02) == 0) {
                oam[addr - 0xFE00] = data;
            }
        } else {
            // Unusable region
            // Pre-CGB devices: writes are ignored
            // CGB: writes are *not* ignored, refer to TCAGBD
            // AGB: writes are ignored
        }
    }
}

u8 Memory::ReadIORegisters(const u16 addr) const {
    switch (addr) {
    // P1 -- Joypad
    case 0xFF00:
        return joypad.p1 | 0xC0;
    // SB -- Serial Data Transfer
    case 0xFF01:
        return serial.serial_data;
    // SC -- Serial control
    case 0xFF02:
        return serial.serial_control | ((game_mode == GameMode::CGB) ? 0x7C : 0x7E);
    // DIV -- Divider Register
    case 0xFF04:
        return static_cast<u8>(timer.divider >> 8);
    // TIMA -- Timer Counter
    case 0xFF05:
        return timer.tima;
    // TMA -- Timer Modulo
    case 0xFF06:
        return timer.tma;
    // TAC -- Timer Control
    case 0xFF07:
        return timer.tac | 0xF8;
    // IF -- Interrupt Flags
    case 0xFF0F:
        return interrupt_flags | 0xE0;
    // NR10 -- Sound Mode 1 Sweep Register
    case 0xFF10:
        return sweep_mode1 | 0x80;
    // NR11 -- Sound Mode 1 Wave Pattern Duty
    case 0xFF11:
        return pattern_duty_mode1 | 0x3F;
    // NR12 -- Sound Mode 1 Envelope
    case 0xFF12:
        return envelope_mode1;
    // NR13 -- Sound Mode 1 Low Frequency
    case 0xFF13:
        return frequency_lo_mode1;
    // NR14 -- Sound Mode 1 High Frequency
    case 0xFF14:
        return frequency_hi_mode1 | 0xBF;
    // NR21 --  Sound Mode 2 Wave Pattern Duty
    case 0xFF16:
        return pattern_duty_mode2 | 0x3F;
    // NR22 --  Sound Mode 2 Envelope
    case 0xFF17:
        return envelope_mode2;
    // NR23 -- Sound Mode 2 Low Frequency
    case 0xFF18:
        return frequency_lo_mode2;
    // NR24 -- Sound Mode 2 High Frequency
    case 0xFF19:
        return frequency_hi_mode2 | 0xBF;
    // NR30 -- Sound Mode 3 On/Off
    case 0xFF1A:
        return sound_on_mode3 | 0x7F;
    // NR31 -- Sound Mode 3 Sound Length
    case 0xFF1B:
        return sound_length_mode3;
    // NR32 -- Sound Mode 3 Select Output
    case 0xFF1C:
        return output_mode3 | 0x9F;
    // NR33 -- Sound Mode 3 Low Frequency
    case 0xFF1D:
        return frequency_lo_mode3;
    // NR34 -- Sound Mode 3 High Frequency
    case 0xFF1E:
        return frequency_hi_mode3 | 0xBF;
    // NR41 -- Sound Mode 4 Sound Length
    case 0xFF20:
        return sound_length_mode4 | 0xE0;
    // NR42 -- Sound Mode 4 Envelope
    case 0xFF21:
        return envelope_mode4;
    // NR43 -- Sound Mode 4 Polynomial Counter
    case 0xFF22:
        return poly_counter_mode4;
    // NR44 -- Sound Mode 4 Counter
    case 0xFF23:
        return counter_mode4 | 0xBF;
    // NR50 -- Channel Control/Volume
    case 0xFF24:
        return volume;
    // NR51 -- Sount Output Terminal Selection
    case 0xFF25:
        return sound_select;
    // NR52 -- Sound On/Off
    case 0xFF26:
        return sound_on | 0x70;
    // Wave Pattern RAM
    case 0xFF30: case 0xFF31: case 0xFF32: case 0xFF33: case 0xFF34: case 0xFF35: case 0xFF36: case 0xFF37:
    case 0xFF38: case 0xFF39: case 0xFF3A: case 0xFF3B: case 0xFF3C: case 0xFF3D: case 0xFF3E: case 0xFF3F:
        return wave_ram[addr - 0xFF30];
    // LCDC -- LCD control
    case 0xFF40:
        return lcd.lcdc;
    // STAT -- LCD status
    case 0xFF41:
        return lcd.stat | 0x80;
    // SCY -- BG Scroll Y
    case 0xFF42:
        return lcd.scroll_y;
    // SCX -- BG Scroll X
    case 0xFF43:
        return lcd.scroll_x;
    // LY -- LCD Current Scanline
    case 0xFF44:
        return lcd.ly;
    // LYC -- LY Compare
    case 0xFF45:
        return lcd.ly_compare;
    // DMA -- OAM DMA Transfer
    case 0xFF46:
        return oam_dma_start;
    // BGP -- BG Palette Data
    case 0xFF47:
        return lcd.bg_palette;
    // OBP0 -- Sprite Palette 0 Data
    case 0xFF48:
        return lcd.obj_palette0;
    // OBP1 -- Sprite Palette 1 Data
    case 0xFF49:
        return lcd.obj_palette1;
    // WY -- Window Y Position
    case 0xFF4A:
        return lcd.window_y;
    // WX -- Window X Position
    case 0xFF4B:
        return lcd.window_x;
    // KEY1 -- Speed Switch
    case 0xFF4D:
        return speed_switch | ((game_mode == GameMode::CGB) ? 0x7E : 0xFF);
    // HDMA5 -- HDMA Length, Mode, and Start
    case 0xFF55:
        return ((game_mode == GameMode::CGB) ? hdma_control : 0xFF);
    // VBK -- VRAM bank number
    case 0xFF4F:
        if (console == Console::CGB) {
            // CGB in DMG mode always has bank 0 selected.
            return ((game_mode == GameMode::CGB) ? (static_cast<u8>(vram_bank_num) | 0xFE) : 0xFE);
        } else {
            return 0xFF;
        }
    // SVBK -- WRAM bank number
    case 0xFF70:
        return ((game_mode == GameMode::CGB) ? (static_cast<u8>(wram_bank_num) | 0xF8) : 0xFF);

    // Unused/unusable I/O registers all return 0xFF when read.
    default:
        return 0xFF;
    }
}

void Memory::WriteIORegisters(const u16 addr, const u8 data) {
    switch (addr) {
    // P1 -- Joypad
    case 0xFF00:
        joypad.p1 = (joypad.p1 & 0x0F) | (data & 0x30);
        break;
    // SB -- Serial Data Transfer
    case 0xFF01:
        serial.serial_data = data;
        break;
    // SC -- Serial control
    case 0xFF02:
        serial.serial_control = data & ((game_mode == GameMode::CGB) ? 0x83 : 0x81);
        break;
    // DIV -- Divider Register
    case 0xFF04:
        // DIV is set to zero on any write.
        timer.divider = 0x0000;
        break;
    // TIMA -- Timer Counter
    case 0xFF05:
        timer.tima = data;
        break;
    // TMA -- Timer Modulo
    case 0xFF06:
        timer.tma = data;
        break;
    // TAC -- Timer Control
    case 0xFF07:
        timer.tac = data & 0x07;
        break;
    // IF -- Interrupt Flags
    case 0xFF0F:
        // If an instruction writes to IF on the same machine cycle an interrupt would have been triggered, the
        // written value remains in IF.
        interrupt_flags = data & 0x1F;
        IF_written_this_cycle = true;
        break;
    // NR10 -- Sound Mode 1 Sweep Register
    case 0xFF10:
        sweep_mode1 = data & 0x7F;
        break;
    // NR11 -- Sound Mode 1 Wave Pattern Duty
    case 0xFF11:
        pattern_duty_mode1 = data;
        break;
    // NR12 -- Sound Mode 1 Envelope
    case 0xFF12:
        envelope_mode1 = data;
        break;
    // NR13 -- Sound Mode 1 Low Frequency
    case 0xFF13:
        frequency_lo_mode1 = data;
        break;
    // NR14 -- Sound Mode 1 High Frequency
    case 0xFF14:
        frequency_hi_mode1 = data & 0xC7;
        break;
    // NR21 --  Sound Mode 2 Wave Pattern Duty
    case 0xFF16:
        pattern_duty_mode2 = data;
        break;
    // NR22 --  Sound Mode 2 Envelope
    case 0xFF17:
        envelope_mode2 = data;
        break;
    // NR23 -- Sound Mode 2 Low Frequency
    case 0xFF18:
        frequency_lo_mode2 = data;
        break;
    // NR24 -- Sound Mode 2 High Frequency
    case 0xFF19:
        frequency_hi_mode2 = data & 0xC7;
        break;
    // NR30 -- Sound Mode 3 On/Off
    case 0xFF1A:
        sound_on_mode3 = data & 0x80;
        break;
    // NR31 -- Sound Mode 3 Sound Length
    case 0xFF1B:
        sound_length_mode3 = data;
        break;
    // NR32 -- Sound Mode 3 Select Output
    case 0xFF1C:
        output_mode3 = data & 0x60;
        break;
    // NR33 -- Sound Mode 3 Low Frequency
    case 0xFF1D:
        frequency_lo_mode3 = data;
        break;
    // NR34 -- Sound Mode 3 High Frequency
    case 0xFF1E:
        frequency_hi_mode3 = data & 0xC7;
        break;
    // NR41 -- Sound Mode 4 Sound Length
    case 0xFF20:
        sound_length_mode4 = data & 0x1F;
        break;
    // NR42 -- Sound Mode 4 Envelope
    case 0xFF21:
        envelope_mode4 = data;
        break;
    // NR43 -- Sound Mode 4 Polynomial Counter
    case 0xFF22:
        poly_counter_mode4 = data;
        break;
    // NR44 -- Sound Mode 4 Counter
    case 0xFF23:
        counter_mode4 = data & 0xC0;
        break;
    // NR50 -- Channel Control/Volume
    case 0xFF24:
        volume = data;
        break;
    // NR51 -- Sount Output Terminal Selection
    case 0xFF25:
        sound_select = data;
        break;
    // NR52 -- Sound On/Off
    case 0xFF26:
        sound_on = data & 0x8F;
        break;
    // Wave Pattern RAM
    case 0xFF30: case 0xFF31: case 0xFF32: case 0xFF33: case 0xFF34: case 0xFF35: case 0xFF36: case 0xFF37:
    case 0xFF38: case 0xFF39: case 0xFF3A: case 0xFF3B: case 0xFF3C: case 0xFF3D: case 0xFF3E: case 0xFF3F:
        wave_ram[addr - 0xFF30] = data;
        break;
    // LCDC -- LCD control
    case 0xFF40:
        lcd.lcdc = data;
        break;
    // STAT -- LCD status
    case 0xFF41:
        lcd.stat = (data & 0x78) | (lcd.stat & 0x07);
        break;
    // SCY -- BG Scroll Y
    case 0xFF42:
        lcd.scroll_y = data;
        break;
    // SCX -- BG Scroll X
    case 0xFF43:
        lcd.scroll_x = data;
        break;
    // LY -- LCD Current Scanline
    case 0xFF44:
        // This register is read only.
        break;
    // LYC -- LY Compare
    case 0xFF45:
        lcd.ly_compare = data;
        break;
    // DMA -- OAM DMA Transfer
    case 0xFF46:
        oam_dma_start = data;
        state_oam_dma = DMAState::RegWritten;
        break;
    // BGP -- BG Palette Data
    case 0xFF47:
        lcd.bg_palette = data;
        break;
    // OBP0 -- Sprite Palette 0 Data
    case 0xFF48:
        lcd.obj_palette0 = data;
        break;
    // OBP1 -- Sprite Palette 1 Data
    case 0xFF49:
        lcd.obj_palette1 = data;
        break;
    // WY -- Window Y Position
    case 0xFF4A:
        lcd.window_y = data;
        break;
    // WX -- Window X Position
    case 0xFF4B:
        lcd.window_x = data;
        break;
    // KEY1 -- Speed Switch
    case 0xFF4D:
        speed_switch = data & 0x01;
        break;
    // HDMA1 -- HDMA Source High Byte
    case 0xFF51:
        hdma_source_hi = data;
        break;
    // HDMA2 -- HDMA Source Low Byte
    case 0xFF52:
        hdma_source_lo = data & 0xF0;
        break;
    // HDMA3 -- HDMA Destination High Byte
    case 0xFF53:
        hdma_dest_hi = data & 0x1F;
        break;
    // HDMA4 -- HDMA Destination Low Byte
    case 0xFF54:
        hdma_dest_lo = data & 0xF0;
        break;
    // HDMA5 -- HDMA Length, Mode, and Start
    case 0xFF55:
        hdma_control = data;
        break;
    // VBK -- VRAM bank number
    case 0xFF4F:
        if (game_mode == GameMode::CGB) {
            vram_bank_num = data & 0x01;
        }
        break;
    // SVBK -- WRAM bank number
    case 0xFF70:
        if (game_mode == GameMode::CGB) {
            wram_bank_num = data & 0x07;
        }
        break;

    default:
        break;
    }
}

} // End namespace Core
