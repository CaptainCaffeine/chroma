// This file is a part of Chroma.
// Copyright (C) 2016-2018 Matthew Murray
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

#include "gb/memory/Memory.h"
#include "gb/memory/CartridgeHeader.h"
#include "gb/memory/Rtc.h"
#include "gb/core/GameBoy.h"
#include "gb/lcd/Lcd.h"
#include "gb/audio/Audio.h"
#include "gb/hardware/Timer.h"
#include "gb/hardware/Serial.h"
#include "gb/hardware/Joypad.h"

namespace Gb {

Memory::Memory(const CartridgeHeader& header, const std::vector<u8>& _rom, const std::string& _save_path,
               GameBoy& _gameboy)
        : gameboy(_gameboy)
        , mbc_mode(header.mbc_mode)
        , ext_ram_present(header.ext_ram_present)
        , rtc_present(header.rtc_present)
        , rumble_present(header.rumble_present)
        , num_rom_banks(header.num_rom_banks)
        , num_ram_banks((header.ram_size) ? std::max(header.ram_size / 0x2000, 1u) : 0)
        , rom(_rom)
        , vram((gameboy.GameModeDmg()) ? 0x2000 : 0x4000)
        , wram((gameboy.GameModeDmg()) ? 0x2000 : 0x8000)
        , hram(0x7F)
        , save_path(_save_path) {

    IORegisterInit();
    VramInit();
    ReadSaveFile(header.ram_size);
    if (rtc_present) {
        rtc = std::make_unique<Rtc>(ext_ram);
    }
}

Memory::~Memory() {
    WriteSaveFile();
}

void Memory::IORegisterInit() {
    if (gameboy.GameModeDmg()) {
        if (gameboy.ConsoleDmg()) {
            gameboy.joypad->p1 = 0xCF; // DMG starts with joypad inputs enabled.
            gameboy.timer->divider = 0xABCC;

            oam_dma_start = 0xFF;

            gameboy.lcd->bg_palette_index = 0xFF;
            gameboy.lcd->obj_palette_index = 0xFF;

            gameboy.lcd->obj_palette_dmg0 = 0xFF;
            gameboy.lcd->obj_palette_dmg1 = 0xFF;
        } else {
            gameboy.joypad->p1 = 0xFF; // CGB starts with joypad inputs disabled, even in DMG mode.
            gameboy.timer->divider = 0x267C;

            oam_dma_start = 0x00;

            gameboy.lcd->bg_palette_index = 0x88;
            gameboy.lcd->obj_palette_index = 0x90;

            gameboy.lcd->obj_palette_dmg0 = 0x00;
            gameboy.lcd->obj_palette_dmg1 = 0x00;
        }
    } else {
        gameboy.joypad->p1 = 0xFF; // Probably?
        gameboy.timer->divider = 0x1EA0;

        oam_dma_start = 0x00;

        gameboy.lcd->bg_palette_index = 0x88;
        gameboy.lcd->obj_palette_index = 0x90;

        gameboy.lcd->obj_palette_dmg0 = 0x00;
        gameboy.lcd->obj_palette_dmg1 = 0x00;
    }

    // I'm assuming the initial value of the internal serial clock is equal to the lower byte of DIV.
    gameboy.serial->InitSerialClock(static_cast<u8>(gameboy.timer->divider));
}

void Memory::VramInit() {
    // The CGB boot ROM does something different.
    if (gameboy.GameModeDmg()) {
        // Initialize the tile map.
        u8 init_tile_map = 0x19;
        vram[0x1910] = init_tile_map--;
        for (std::size_t addr = 0x192F; addr >= 0x1924; --addr) {
            vram[addr] = init_tile_map--;
        }

        for (std::size_t addr = 0x190F; addr >= 0x1904; --addr) {
            vram[addr] = init_tile_map--;
        }
    }

    const std::array<u8, 200> init_tile_data{{
        0xF0, 0xF0, 0xFC, 0xFC, 0xFC, 0xFC, 0xF3, 0xF3,
        0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C,
        0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0xF3, 0xF3,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCF, 0xCF,
        0x00, 0x00, 0x0F, 0x0F, 0x3F, 0x3F, 0x0F, 0x0F,
        0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0x0F, 0x0F,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF3, 0xF3,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0xFF, 0xFF,
        0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC3, 0xC3,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFC,
        0xF3, 0xF3, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
        0x3C, 0x3C, 0xFC, 0xFC, 0xFC, 0xFC, 0x3C, 0x3C,
        0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3,
        0xF3, 0xF3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3,
        0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF,
        0x3C, 0x3C, 0x3F, 0x3F, 0x3C, 0x3C, 0x0F, 0x0F,
        0x3C, 0x3C, 0xFC, 0xFC, 0x00, 0x00, 0xFC, 0xFC,
        0xFC, 0xFC, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
        0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF0, 0xF0,
        0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF,
        0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xC3, 0xC3,
        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFC, 0xFC,
        0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C
    }};

    // The boot ROM only writes to every other address, starting at 0x8010.
    auto tile_data_iter = init_tile_data.begin();
    for (std::size_t addr = 0x0010; addr < 0x01A0; addr += 2) {
        vram[addr] = *tile_data_iter++;
    }
}

u8 Memory::ReadMem(const u16 addr) const {
    if (addr < 0x8000) {
        // ROM
        if (dma_bus_block != Bus::External) {
            if (addr < 0x4000) {
                // ROM0 bank
                if (mbc_mode == MBC::MBC1) {
                    return rom[addr + 0x4000 * ((ram_bank_num << 5) & (num_rom_banks - 1))];
                } else if (mbc_mode == MBC::MBC1M) {
                    return rom[addr + 0x4000 * ((ram_bank_num << 4) & (num_rom_banks - 1))];
                } else {
                    return rom[addr];
                }
            } else {
                // ROM1 bank
                return rom[addr + 0x4000 * ((rom_bank_num & (num_rom_banks - 1)) - 1)];
            }
        } else {
            // If OAM DMA is currently transferring from the external bus, return the last byte read by the DMA.
            return oam_transfer_byte;
        }
    } else if (addr < 0xA000) {
        // VRAM -- switchable in CGB mode
        if (dma_bus_block != Bus::Vram) {
            // Not accessible during screen mode 3.
            if ((gameboy.lcd->stat & 0x03) != 3) {
                return vram[addr - 0x8000 + 0x2000 * vram_bank_num];
            } else {
                return 0xFF;
            }
        } else {
            // If OAM DMA is currently transferring from VRAM, return the last byte read by the DMA.
            return oam_transfer_byte;
        }
    } else if (addr < 0xFE00) {
        if (dma_bus_block != Bus::External) {
            if (addr < 0xC000) {
                // External RAM bank.
                return ReadExternalRam(addr);
            } else if (addr < 0xD000) {
                // WRAM bank 0
                return wram[addr - 0xC000];
            } else if (addr < 0xE000) {
                // WRAM bank 1 (switchable from 1-7 in CGB mode)
                return wram[addr - 0xC000 + 0x1000 * ((wram_bank_num == 0) ? 0 : wram_bank_num - 1)];
            } else if (addr < 0xF000) {
                // Echo of C000-DDFF
                return wram[addr - 0xE000];
            } else {
                // Echo of C000-DDFF
                return wram[addr - 0xE000 + 0x1000 * ((wram_bank_num == 0) ? 0 : wram_bank_num - 1)];
            }
        } else {
            // If OAM DMA is currently transferring from the external bus, return the last byte read by the DMA.
            return oam_transfer_byte;
        }
    } else if (addr < 0xFF00) {
        if (addr < 0xFEA0) {
            // OAM (Sprite Attribute Table)
            if (dma_bus_block == Bus::None && !(gameboy.lcd->stat & 0x02)) {
                return gameboy.lcd->oam[addr - 0xFE00];
            } else {
                // Inaccessible during OAM DMA, and during screen modes 2 and 3.
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
        // 0xFF00-0xFFFF is still accessible during OAM DMA.
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
    }
}

void Memory::WriteMem(const u16 addr, const u8 data) {
    if (addr < 0x8000) {
        // MBC control registers -- writes to this region do not write the ROM.
        // If OAM DMA is currently transferring from the external bus, the write is ignored.
        if (dma_bus_block != Bus::External) {
            WriteMbcControlRegisters(addr, data);
        }
    } else if (addr < 0xA000) {
        // VRAM -- switchable in CGB mode
        // If OAM DMA is currently transferring from the VRAM bus, the write is ignored.
        if (dma_bus_block != Bus::Vram && (gameboy.lcd->stat & 0x03) != 3) {
            // Not accessible during screen mode 3.
            vram[addr - 0x8000 + 0x2000 * vram_bank_num] = data;
        }
    } else if (addr < 0xFE00) {
        // If OAM DMA is currently transferring from the external bus, the write is ignored.
        if (dma_bus_block != Bus::External) {
            if (addr < 0xC000) {
                // External RAM bank.
                WriteExternalRam(addr, data);
            } else if (addr < 0xD000) {
                // WRAM bank 0
                wram[addr - 0xC000] = data;
            } else if (addr < 0xE000) {
                // WRAM bank 1 (switchable from 1-7 in CGB mode)
                wram[addr - 0xC000 + 0x1000 * ((wram_bank_num == 0) ? 0 : wram_bank_num - 1)] = data;
            } else if (addr < 0xF000) {
                // Echo of C000-DDFF
                wram[addr - 0xE000] = data;
            } else {
                // Echo of C000-DDFF
                wram[addr - 0xE000 + 0x1000 * ((wram_bank_num == 0) ? 0 : wram_bank_num - 1)] = data;
            }
        }
    } else if (addr < 0xFF00) {
        // OAM (Sprite Attribute Table)
        // Inaccessible during OAM DMA.
        if (dma_bus_block == Bus::None && addr < 0xFEA0) {
            // Inaccessible during screen modes 2 and 3.
            if (!(gameboy.lcd->stat & 0x02)) {
                gameboy.lcd->oam[addr - 0xFE00] = data;
            }
        }
        // 0xFEA0-0xFEFF: Unusable region
        // Pre-CGB devices: writes are ignored
        // CGB: writes are *not* ignored, refer to TCAGBD
        // AGB: writes are ignored
    } else {
        // 0xFF00-0xFFFF is still accessible during OAM DMA.
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
    }
}

u8 Memory::ReadIORegisters(const u16 addr) const {
    switch (addr) {
    case P1:
        return gameboy.joypad->p1 | 0xC0;
    case SB:
        return gameboy.serial->serial_data;
    case SC:
        return gameboy.serial->serial_control | ((gameboy.GameModeCgb()) ? 0x7C : 0x7E);
    case DIV:
        return static_cast<u8>(gameboy.timer->divider >> 8);
    case TIMA:
        return gameboy.timer->tima;
    case TMA:
        return gameboy.timer->tma;
    case TAC:
        return gameboy.timer->tac | 0xF8;
    case IF:
        return interrupt_flags | 0xE0;
    case NR10:
        return gameboy.audio->square1.ReadSweepCgb();
    case NR11:
        return gameboy.audio->square1.ReadSoundLengthCgb();
    case NR12:
        return gameboy.audio->square1.ReadEnvelopeCgb();
    case NR13:
        // This register is write-only.
        return 0xFF;
    case NR14:
        return gameboy.audio->square1.ReadResetCgb();
    case NR21:
        return gameboy.audio->square2.ReadSoundLengthCgb();
    case NR22:
        return gameboy.audio->square2.ReadEnvelopeCgb();
    case NR23:
        // This register is write-only.
        return 0xFF;
    case NR24:
        return gameboy.audio->square2.ReadResetCgb();
    case NR30:
        return gameboy.audio->wave.ReadSweepCgb();
    case NR31:
        // This register is write-only.
        return 0xFF;
    case NR32:
        return gameboy.audio->wave.ReadEnvelopeCgb();
    case NR33:
        // This register is write-only.
        return 0xFF;
    case NR34:
        return gameboy.audio->wave.ReadResetCgb();
    case NR41:
        // This register is write-only.
        return 0xFF;
    case NR42:
        return gameboy.audio->noise.ReadEnvelopeCgb();
    case NR43:
        return gameboy.audio->noise.ReadNoiseControlCgb();
    case NR44:
        return gameboy.audio->noise.ReadResetCgb();
    case NR50:
        return gameboy.audio->master_volume;
    case NR51:
        return gameboy.audio->sound_select;
    case NR52:
        return gameboy.audio->ReadSoundOn();
    case WAVE_0: case WAVE_1: case WAVE_2: case WAVE_3: case WAVE_4: case WAVE_5: case WAVE_6: case WAVE_7:
    case WAVE_8: case WAVE_9: case WAVE_A: case WAVE_B: case WAVE_C: case WAVE_D: case WAVE_E: case WAVE_F:
        return gameboy.audio->wave_ram[addr - WAVE_0];
    case LCDC:
        return gameboy.lcd->lcdc;
    case STAT:
        return gameboy.lcd->stat | 0x80;
    case SCY:
        return gameboy.lcd->scroll_y;
    case SCX:
        return gameboy.lcd->scroll_x;
    case LY:
        return gameboy.lcd->ly;
    case LYC:
        return gameboy.lcd->ly_compare;
    case DMA:
        return oam_dma_start;
    case BGP:
        return gameboy.lcd->bg_palette_dmg;
    case OBP0:
        return gameboy.lcd->obj_palette_dmg0;
    case OBP1:
        return gameboy.lcd->obj_palette_dmg1;
    case WY:
        return gameboy.lcd->window_y;
    case WX:
        return gameboy.lcd->window_x;
    case KEY1:
        return speed_switch | ((gameboy.GameModeCgb()) ? 0x7E : 0xFF);
    case VBK:
        if (gameboy.ConsoleCgb()) {
            // CGB in DMG mode always has bank 0 selected.
            return ((gameboy.GameModeCgb()) ? (static_cast<u8>(vram_bank_num) | 0xFE) : 0xFE);
        } else {
            return 0xFF;
        }
    case HDMA1:
        // This register is write-only.
        return 0xFF;
    case HDMA2:
        // This register is write-only.
        return 0xFF;
    case HDMA3:
        // This register is write-only.
        return 0xFF;
    case HDMA4:
        // This register is write-only.
        return 0xFF;
    case HDMA5:
        return ((gameboy.GameModeCgb()) ? hdma_control : 0xFF);
    case RP:
        return ((gameboy.GameModeCgb()) ? (infrared | 0x3C) : 0xFF);
    case BGPI:
        if (gameboy.ConsoleCgb()) {
            return gameboy.lcd->bg_palette_index | 0x40;
        } else {
            return 0xFF;
        }
    case BGPD:
        // Palette RAM is not accessible during mode 3.
        if (gameboy.GameModeCgb() && (gameboy.lcd->stat & 0x03) != 3) {
            return gameboy.lcd->bg_palette_data[gameboy.lcd->bg_palette_index & 0x3F];
        } else {
            return 0xFF;
        }
    case OBPI:
        if (gameboy.ConsoleCgb()) {
            return gameboy.lcd->obj_palette_index | 0x40;
        } else {
            return 0xFF;
        }
    case OBPD:
        // Palette RAM is not accessible during mode 3.
        if (gameboy.GameModeCgb() && (gameboy.lcd->stat & 0x03) != 3) {
            return gameboy.lcd->obj_palette_data[gameboy.lcd->obj_palette_index & 0x3F];
        } else {
            return 0xFF;
        }
    case SVBK:
        return ((gameboy.GameModeCgb()) ? (static_cast<u8>(wram_bank_num) | 0xF8) : 0xFF);
    case UNDOC0:
        return ((gameboy.GameModeCgb()) ? (undocumented[0] | 0xFE) : 0xFF);
    case UNDOC1:
        return ((gameboy.ConsoleCgb()) ? undocumented[1] : 0xFF);
    case UNDOC2:
        return ((gameboy.ConsoleCgb()) ? undocumented[2] : 0xFF);
    case UNDOC3:
        return ((gameboy.GameModeCgb()) ? undocumented[3] : 0xFF);
    case UNDOC4:
        return ((gameboy.ConsoleCgb()) ? (undocumented[4] | 0x8F) : 0xFF);
    case UNDOC5:
    case UNDOC6:
        return ((gameboy.ConsoleCgb()) ? 0x00 : 0xFF);

    // Unused/unusable I/O registers all return 0xFF when read.
    default:
        return 0xFF;
    }
}

void Memory::WriteIORegisters(const u16 addr, const u8 data) {
    switch (addr) {
    case P1:
        gameboy.joypad->p1 = (gameboy.joypad->p1 & 0x0F) | (data & 0x30);
        gameboy.joypad->UpdateJoypad();
        break;
    case SB:
        gameboy.serial->serial_data = data;
        break;
    case SC:
        gameboy.serial->serial_control = data & ((gameboy.GameModeCgb()) ? 0x83 : 0x81);
        break;
    case DIV:
        // DIV is set to zero on any write.
        gameboy.timer->divider = 0x0000;
        break;
    case TIMA:
        gameboy.timer->tima = data;
        break;
    case TMA:
        gameboy.timer->tma = data;
        break;
    case TAC:
        gameboy.timer->tac = data & 0x07;
        break;
    case IF:
        // If an instruction writes to IF on the same machine cycle an interrupt would have been triggered, the
        // written value remains in IF.
        interrupt_flags = data & 0x1F;
        IF_written_this_cycle = true;
        break;
    case NR10:
    case NR11:
    case NR12:
    case NR13:
    case NR14:
    case NR21:
    case NR22:
    case NR23:
    case NR24:
    case NR30:
    case NR31:
    case NR32:
    case NR33:
    case NR34:
    case NR41:
    case NR42:
    case NR43:
    case NR44:
    case NR50:
    case NR51:
    case NR52:
    case WAVE_0: case WAVE_1: case WAVE_2: case WAVE_3: case WAVE_4: case WAVE_5: case WAVE_6: case WAVE_7:
    case WAVE_8: case WAVE_9: case WAVE_A: case WAVE_B: case WAVE_C: case WAVE_D: case WAVE_E: case WAVE_F:
        gameboy.audio->WriteSoundRegs(addr, data);
        break;
    case LCDC:
        gameboy.lcd->WriteLcdc(data);
        break;
    case STAT:
        gameboy.lcd->stat = (data & 0x78) | (gameboy.lcd->stat & 0x07);
        // On DMG, if the STAT register is written during mode 1 or 0 while the LCD is on, bit 1 of the IF register
        // is set. This causes a STAT interrupt if it's enabled in IE.
        if (gameboy.ConsoleDmg() && (gameboy.lcd->lcdc & 0x80) && !(gameboy.lcd->stat & 0x02)) {
            gameboy.lcd->SetStatSignal();
        }
        break;
    case SCY:
        gameboy.lcd->scroll_y = data;
        break;
    case SCX:
        gameboy.lcd->scroll_x = data;
        break;
    case LY:
        // This register is read only.
        break;
    case LYC:
        gameboy.lcd->ly_compare = data;
        break;
    case DMA:
        oam_dma_start = data;
        oam_dma_state = DmaState::Starting;
        break;
    case BGP:
        gameboy.lcd->bg_palette_dmg = data;
        break;
    case OBP0:
        gameboy.lcd->obj_palette_dmg0 = data;
        break;
    case OBP1:
        gameboy.lcd->obj_palette_dmg1 = data;
        break;
    case WY:
        gameboy.lcd->WriteWy(data);
        break;
    case WX:
        gameboy.lcd->WriteWx(data);
        break;
    case KEY1:
        speed_switch = (speed_switch & 0x80) | (data & 0x01);
        break;
    case VBK:
        if (gameboy.GameModeCgb()) {
            vram_bank_num = data & 0x01;
        }
        break;
    case HDMA1:
        hdma_source_hi = data;
        break;
    case HDMA2:
        hdma_source_lo = data & 0xF0;
        break;
    case HDMA3:
        hdma_dest_hi = data & 0x1F;
        break;
    case HDMA4:
        hdma_dest_lo = data & 0xF0;
        break;
    case HDMA5:
        hdma_control = data;
        if (gameboy.GameModeCgb()) {
            hdma_reg_written = true;
        }
        break;
    case RP:
        if (gameboy.GameModeCgb()) {
            infrared = (infrared & 0x02) | (data & 0xC1);
        }
        break;
    case BGPI:
        if (gameboy.GameModeCgb()) {
            gameboy.lcd->bg_palette_index = data & 0xBF;
        }
        break;
    case BGPD:
        // Palette RAM is not accessible during mode 3.
        if (gameboy.GameModeCgb() && (gameboy.lcd->stat & 0x03) != 3) {
            gameboy.lcd->bg_palette_data[gameboy.lcd->bg_palette_index & 0x3F] = data;
            // Increment index if auto-increment specified.
            if (gameboy.lcd->bg_palette_index & 0x80) {
                gameboy.lcd->bg_palette_index = (gameboy.lcd->bg_palette_index + 1) & 0xBF;
            }
        }
        break;
    case OBPI:
        if (gameboy.GameModeCgb()) {
            gameboy.lcd->obj_palette_index = data & 0xBF;
        }
        break;
    case OBPD:
        // Palette RAM is not accessible during mode 3.
        if (gameboy.GameModeCgb() && (gameboy.lcd->stat & 0x03) != 3) {
            gameboy.lcd->obj_palette_data[gameboy.lcd->obj_palette_index & 0x3F] = data;
            // Increment index if auto-increment specified.
            if (gameboy.lcd->obj_palette_index & 0x80) {
                gameboy.lcd->obj_palette_index = (gameboy.lcd->obj_palette_index + 1) & 0xBF;
            }
        }
        break;
    case SVBK:
        if (gameboy.GameModeCgb()) {
            wram_bank_num = data & 0x07;
        }
        break;
    case UNDOC0:
        if (gameboy.GameModeCgb()) {
            undocumented[0] = data & 0x01;
        }
        break;
    case UNDOC1:
        if (gameboy.ConsoleCgb()) {
            undocumented[1] = data;
        }
        break;
    case UNDOC2:
        if (gameboy.ConsoleCgb()) {
            undocumented[2] = data;
        }
        break;
    case UNDOC3:
        if (gameboy.GameModeCgb()) {
            undocumented[3] = data;
        }
        break;
    case UNDOC4:
        if (gameboy.ConsoleCgb()) {
            undocumented[4] = data & 0x70;
        }
        break;
    case UNDOC5:
        // This register is read only.
        break;
    case UNDOC6:
        // This register is read only.
        break;

    default:
        break;
    }
}

} // End namespace Gb
