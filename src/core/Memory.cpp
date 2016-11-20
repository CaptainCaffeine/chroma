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

#include <cassert>
#include <vector>

#include "core/Memory.h"

namespace Core {

Memory::Memory(const Console game_boy, const CartridgeHeader cart_header, std::vector<u8> rom_contents)
        : console(game_boy)
        , game_mode(cart_header.game_mode)
        , mbc_mode(cart_header.mbc_mode)
        , ext_ram_present(cart_header.ext_ram_present)
        , num_rom_banks(cart_header.num_rom_banks)
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
        ext_ram = std::vector<u8>(cart_header.ram_size);
    }

    // 160 bytes object attribute memory.
    oam = std::vector<u8>(0xA0);
    // 127 bytes high RAM (fast-access internal ram) + interrupt enable register.
    hram = std::vector<u8>(0x80);

    IORegisterInit();
}

void Memory::IORegisterInit() {
    if (game_mode == GameMode::DMG) {
        if (console == Console::DMG) {
            joypad = 0xCF;
            divider = 0xABCC;
        } else {
            joypad = 0xFF;
            divider = 0x267C;
        }
    } else {
        joypad = 0xFF; // Probably?
        divider = 0x1EA0;
    }
}

u8 Memory::ReadMem8(const u16 addr) const {
    if (addr < 0x4000) {
        // Fixed ROM bank
        return rom[addr];
    } else if (addr < 0x8000) {
        // Switchable ROM bank.
        return rom[addr + 0x4000*((rom_bank_num % num_rom_banks) - 1)];
    } else if (addr < 0xA000) {
        // VRAM -- switchable in CGB mode
        return vram[addr - 0x8000 + 0x2000*vram_bank_num];
    } else if (addr < 0xC000) {
        // External RAM bank.
        if (ext_ram_enabled) {
            if (mbc_mode == MBC::MBC1) {
                // Out of bounds reads return 0xFF.
                u16 adjusted_addr = addr - 0xA000 + 0x2000*ram_bank_num;
                if (adjusted_addr < ext_ram.size()) {
                    return ext_ram[adjusted_addr];
                } else {
                    return 0xFF;
                }
            } else if (mbc_mode == MBC::MBC2) {
                // MBC2 RAM range is only A000-A1FF.
                u16 adjusted_addr = addr - 0xA000;
                if (adjusted_addr < ext_ram.size()) {
                    return ext_ram[adjusted_addr] & 0xF0;
                } else {
                    return 0xFF;
                }
            } else {
                // Currently only MBC1 and MBC2 supported. Default value returned here to silence a warning.
                return 0xFF;
            }
        } else {
            // Reads from this region when the RAM banks are disabled or not present return 0xFF.
            return 0xFF;
        }
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
        return oam[addr - 0xFE00];
    } else if (addr < 0xFF00) {
        // Unusable region
        // Pre-CGB devices: reads return 0x00
        // CGB: reads vary, refer to TCAGBD
        // AGB: reads return 0xNN where N is the high nybble of the lower byte of addr.
        return 0x00;
    } else if (addr < 0xFF80) {
        // I/O registers
        return ReadIORegisters(addr);
    } else {
        // High RAM + interrupt enable (IE) register at 0xFFFF
        return hram[addr - 0xFF80];
    }
}

u16 Memory::ReadMem16(const u16 addr) const {
    u8 byte_lo = ReadMem8(addr);
    u8 byte_hi = ReadMem8(addr+1);

    return (static_cast<u16>(byte_hi) << 8) | static_cast<u16>(byte_lo);
}

void Memory::WriteMem8(const u16 addr, const u8 data) {
    if (addr < 0x8000) {
        // MBC control registers -- writes to this region do not write the ROM.
        WriteMBCControlRegisters(addr, data);
    } else if (addr < 0xA000) {
        // VRAM -- switchable in CGB mode
        if (game_mode == GameMode::CGB) {
            vram[addr - 0x8000 + 0x2000*vram_bank_num] = data;
        }
        vram[addr - 0x8000] = data;
    } else if (addr < 0xC000) {
        // External RAM bank, switchable.
        if (ext_ram_enabled) {
            if (mbc_mode == MBC::MBC1) {
                // There can apparently be varying ram sizes here for MBC1, may need to make this more fine-grained.
                // Ignore out-of-bounds writes.
                u16 adjusted_addr = addr - 0xA000 + 0x2000*ram_bank_num;
                if (adjusted_addr < ext_ram.size()) {
                    ext_ram[adjusted_addr] = data;
                }
            } else if (mbc_mode == MBC::MBC2) {
                // MBC2 RAM range is only A000-A1FF. Only the lower nibble of the bytes in this region are used.
                u16 adjusted_addr = addr - 0xA000;
                if (adjusted_addr < ext_ram.size()) {
                    ext_ram[adjusted_addr] = data & 0x0F;
                }
            }
        }
        // Writes are ignored if the RAM banks are not enabled.
    } else if (addr < 0xD000) {
        // WRAM bank 0
        wram[addr - 0xC000] = data;
    } else if (addr < 0xE000) {
        // WRAM bank 1 (switchable from 1-7 in CGB mode)
        wram[addr - 0xC000 + 0x1000*((wram_bank_num == 0) ? 0 : wram_bank_num-1)] = data;
    } else if (addr < 0xFE00) {
        // Echo of C000-DDFF
        wram[addr - 0xE000 + 0x1000*((wram_bank_num == 0) ? 0 : wram_bank_num-1)] = data;
        // For some unlicensed games and flashcarts on pre-CGB devices, writes to this region write to both WRAM and 
        // external RAM (source: AntonioND timing docs).
    } else if (addr < 0xFEA0) {
        // OAM (Sprite Attribute Table)
        oam[addr - 0xFE00] = data;
    } else if (addr < 0xFF00) {
        // Unusable region
        // Pre-CGB devices: writes are ignored
        // CGB: writes are *not* ignored, refer to TCAGBD
        // AGB: writes are ignored
    } else if (addr < 0xFF80) {
        // I/O registers
        WriteIORegisters(addr, data);
    } else {
        // High RAM + interrupt enable (IE) register.
        hram[addr - 0xFF80] = data;
    }
}

void Memory::WriteMem16(const u16 addr, const u16 data) {
    WriteMem8(addr, static_cast<u8>(data));
    WriteMem8(addr+1, static_cast<u8>(data >> 8));
}

u8 Memory::ReadIORegisters(const u16 addr) const {
    switch (addr) {
    // P1 -- Joypad
    case 0xFF00:
        return joypad | 0xC0;
    // SB -- Serial Data Transfer
    case 0xFF01:
        return serial_data;
    // SC -- Serial control
    case 0xFF02:
        return serial_control | ((game_mode == GameMode::CGB) ? 0x7C : 0x7E);
    // DIV -- Divider Register
    case 0xFF04:
        return static_cast<u8>(divider >> 8);
    // TIMA -- Timer Counter
    case 0xFF05:
        return timer_counter;
    // TMA -- Timer Modulo
    case 0xFF06:
        return timer_modulo;
    // TAC -- Timer Control
    case 0xFF07:
        return timer_control | 0xF8;
    // IF -- Interrupt Flags
    case 0xFF0F:
        return interrupt_flags | 0xE0;
    // LCDC -- LCD control
    case 0xFF40:
        return lcd_control;
    // STAT -- LCD status
    case 0xFF41:
        return lcd_status | 0x80;
    // SCY -- BG Scroll Y
    case 0xFF42:
        return scroll_y;
    // SCX -- BG Scroll X
    case 0xFF43:
        return scroll_x;
    // LY -- LCD Current Scanline
    case 0xFF44:
        return lcd_current_scanline;
    // LYC -- LY Compare
    case 0xFF45:
        return ly_compare;
    // DMA -- OAM DMA Transfer
    case 0xFF46:
        return oam_dma; // Is this register write only?
    // BGP -- BG Palette Data
    case 0xFF47:
        return bg_palette_data;
    // OBP0 -- Sprite Palette 0 Data
    case 0xFF48:
        return obj_palette0_data;
    // OBP1 -- Sprite Palette 1 Data
    case 0xFF49:
        return obj_palette1_data;
    // WY -- Window Y Position
    case 0xFF4A:
        return window_y_pos;
    // WX -- Window X Position
    case 0xFF4B:
        return window_x_pos;
    // KEY1 -- Speed Switch
    case 0xFF4D:
        return speed_switch | ((game_mode == GameMode::CGB) ? 0x7E : 0xFF);
    // HDMA5 -- HDMA Length, Mode, and Start
    case 0xFF55:
        return ((game_mode == GameMode::CGB) ? hdma_control : 0xFF);
    // VBK -- VRAM bank number
    case 0xFF4F:
        if (console == Console::CGB) {
            if (game_mode == GameMode::DMG) {
                // GBC in DMG mode always has bank 0 selected.
                return 0xFE;
            } else {
                return static_cast<u8>(vram_bank_num) | 0xFE;
            }
        } else {
            return 0xFF;
        }
    // SVBK -- WRAM bank number
    case 0xFF70:
        if (game_mode == GameMode::DMG) {
            return 0xFF;
        } else {
            return static_cast<u8>(wram_bank_num) | 0xF8;
        }

    // Unused/unusable I/O registers all return 0xFF when read.
    default:
        return 0xFF;
    }
}

void Memory::WriteIORegisters(const u16 addr, const u8 data) {
    switch (addr) {
    // P1 -- Joypad
    case 0xFF00:
        joypad = data & 0x30;
        break;
    // SB -- Serial Data Transfer
    case 0xFF01:
        serial_data = data;
        break;
    // SC -- Serial control
    case 0xFF02:
        serial_control = data & ((game_mode == GameMode::CGB) ? 0x83 : 0x81);
        break;
    // DIV -- Divider Register
    case 0xFF04:
        // DIV is set to zero on any write.
        divider = 0x0000;
        break;
    // TIMA -- Timer Counter
    case 0xFF05:
        timer_counter = data;
        break;
    // TMA -- Timer Modulo
    case 0xFF06:
        timer_modulo = data;
        break;
    // TAC -- Timer Control
    case 0xFF07:
        timer_control = data & 0x07;
        break;
    // IF -- Interrupt Flags
    case 0xFF0F:
        // If an instruction writes to IF on the same machine cycle an interrupt would have been triggered, the
        // written value remains in IF.
        if (!IF_written_this_cycle) {
            interrupt_flags = data & 0x1F;
            IF_written_this_cycle = true;
        }
        break;
    // LCDC -- LCD control
    case 0xFF40:
        lcd_control = data;
        break;
    // STAT -- LCD status
    case 0xFF41:
        lcd_status = data & 0x78;
        break;
    // SCY -- BG Scroll Y
    case 0xFF42:
        scroll_y = data;
        break;
    // SCX -- BG Scroll X
    case 0xFF43:
        scroll_x = data;
        break;
    // LY -- LCD Current Scanline
    case 0xFF44:
        // This register is read only.
        break;
    // LYC -- LY Compare
    case 0xFF45:
        ly_compare = data;
        break;
    // DMA -- OAM DMA Transfer
    case 0xFF46:
        oam_dma = data;
        // TODO: Trigger the actual DMA copy
        break;
    // BGP -- BG Palette Data
    case 0xFF47:
        bg_palette_data = data;
        break;
    // OBP0 -- Sprite Palette 0 Data
    case 0xFF48:
        obj_palette0_data = data;
        break;
    // OBP1 -- Sprite Palette 1 Data
    case 0xFF49:
        obj_palette1_data = data;
        break;
    // WY -- Window Y Position
    case 0xFF4A:
        window_y_pos = data;
        break;
    // WX -- Window X Position
    case 0xFF4B:
        window_x_pos = data;
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

void Memory::WriteMBCControlRegisters(const u16 addr, const u8 data) {
    switch (mbc_mode) {
    case MBC::MBC1:
        if (addr < 0x2000) {
            // RAM enable register -- RAM banking is enabled if a byte with lower nibble 0xA is written
            if (ext_ram_present && (data & 0x0F) == 0x0A) {
                ext_ram_enabled = true;
            } else {
                ext_ram_enabled = false;
            }
        } else if (addr < 0x4000) {
            // ROM bank register
            // Only the lower 5 bits of the written value are considered -- preserve the upper bits.
            rom_bank_num = (rom_bank_num & 0x60) | (data & 0x1F);

            // 0x00, 0x20, 0x40, 0x60 all map to 0x01, 0x21, 0x41, 0x61 respectively.
            if (rom_bank_num == 0x00 || rom_bank_num == 0x20 || rom_bank_num == 0x40 || rom_bank_num == 0x60) {
                ++rom_bank_num;
            }
        } else if (addr < 0x6000) {
            // RAM bank register (or upper bits ROM bank)
            // Only the lower 2 bits of the written value are considered.
            if (ram_bank_mode) {
                ram_bank_num = data & 0x03;
            } else {
                rom_bank_num = (rom_bank_num & 0x1F) | (data & 0x03) << 5;
            }
        } else if (addr < 0x8000) {
            // Memory mode -- selects whether the two bits in the above register act as the RAM bank number or 
            // the upper bits of the ROM bank number.
            ram_bank_mode = data & 0x01;
            if (ram_bank_mode) {
                // The 5th and 6th bits of the ROM bank number become the RAM bank number.
                ram_bank_num = (rom_bank_num & 0x60) >> 5;
                rom_bank_num &= 0x1F;
            } else {
                // The RAM bank number becomes the 5th and 6th bits of the ROM bank number.
                rom_bank_num |= ram_bank_num << 5;
                ram_bank_num = 0x00;
            }
        }
        break;
    case MBC::MBC2:
        if (addr < 0x2000) {
            // RAM enable register -- RAM banking is enabled if a byte with lower nibble 0xA is written
            // The least significant bit of the upper address byte must be zero to enable or disable external ram.
            if (!(addr & 0x0100)) {
                if (ext_ram_present && (data & 0x0F) == 0x0A) {
                    ext_ram_enabled = true;
                } else {
                    ext_ram_enabled = false;
                }
            }
        } else if (addr < 0x4000) {
            // ROM bank register -- The least significant bit of the upper address byte must be 1 to switch ROM banks.
            if (addr & 0x0100) {
                // Only the lower 4 bits of the written value are considered.
                rom_bank_num = data & 0x0F;
                if (rom_bank_num == 0) {
                    ++rom_bank_num;
                }
            }
        }
        // MBC2 does not have RAM banking.
        break;
    default:
        // Carts with no MBC ignore writes here.
        break;
    }
}

} // End namespace Core
