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

#include "gb/memory/CartridgeHeader.h"
#include "gb/memory/Memory.h"
#include "gb/memory/RTC.h"
#include "gb/lcd/LCD.h"
#include "gb/audio/Audio.h"
#include "gb/hardware/Timer.h"
#include "gb/hardware/Serial.h"
#include "gb/hardware/Joypad.h"

namespace Gb {

Memory::Memory(const Console gb_type, const CartridgeHeader& header, Timer& tima, Serial& sio, LCD& display,
               Joypad& pad, Audio& apu, const std::vector<u8>& rom_contents, std::vector<u8>& save_game)
        : console(gb_type)
        , game_mode(header.game_mode)
        , timer(tima)
        , serial(sio)
        , lcd(display)
        , joypad(pad)
        , audio(apu)
        , mbc_mode(header.mbc_mode)
        , ext_ram_present(header.ext_ram_present)
        , rtc_present(header.rtc_present)
        , rumble_present(header.rumble_present)
        , num_rom_banks(header.num_rom_banks)
        , num_ram_banks((header.ram_size) ? std::max(header.ram_size / 0x2000, 1u) : 0)
        , rom(rom_contents)
        , vram((game_mode == GameMode::DMG) ? 0x2000 : 0x4000)
        , wram((game_mode == GameMode::DMG) ? 0x2000 : 0x8000)
        , hram(0x7F)
        , ext_ram(save_game)
        , rtc((rtc_present) ? std::make_unique<RTC>(ext_ram) : nullptr) {

    IORegisterInit();
    VRAMInit();
}

// Needed to declare std::unique_ptr with forward-declared type in the header file.
Memory::~Memory() = default;

void Memory::IORegisterInit() {
    if (game_mode == GameMode::DMG) {
        if (IsConsoleDmg()) {
            joypad.p1 = 0xCF; // DMG starts with joypad inputs enabled.
            timer.divider = 0xABCC;

            oam_dma_start = 0xFF;

            lcd.bg_palette_index = 0xFF;
            lcd.obj_palette_index = 0xFF;

            lcd.obj_palette_dmg0 = 0xFF;
            lcd.obj_palette_dmg1 = 0xFF;
        } else {
            joypad.p1 = 0xFF; // CGB starts with joypad inputs disabled, even in DMG mode.
            timer.divider = 0x267C;

            oam_dma_start = 0x00;

            lcd.bg_palette_index = 0x88;
            lcd.obj_palette_index = 0x90;

            lcd.obj_palette_dmg0 = 0x00;
            lcd.obj_palette_dmg1 = 0x00;
        }
    } else {
        joypad.p1 = 0xFF; // Probably?
        timer.divider = 0x1EA0;

        oam_dma_start = 0x00;

        lcd.bg_palette_index = 0x88;
        lcd.obj_palette_index = 0x90;

        lcd.obj_palette_dmg0 = 0x00;
        lcd.obj_palette_dmg1 = 0x00;
    }

    // I'm assuming the initial value of the internal serial clock is equal to the lower byte of DIV.
    serial.InitSerialClock(static_cast<u8>(timer.divider));
}

void Memory::VRAMInit() {
    // The CGB boot ROM does something different.
    if (game_mode == GameMode::DMG) {
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
        if (dma_bus_block != Bus::VRAM) {
            // Not accessible during screen mode 3.
            if ((lcd.stat & 0x03) != 3) {
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
                return ReadExternalRAM(addr);
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
            if (dma_bus_block == Bus::None && !(lcd.stat & 0x02)) {
                return lcd.oam[addr - 0xFE00];
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
            WriteMBCControlRegisters(addr, data);
        }
    } else if (addr < 0xA000) {
        // VRAM -- switchable in CGB mode
        // If OAM DMA is currently transferring from the VRAM bus, the write is ignored.
        if (dma_bus_block != Bus::VRAM && (lcd.stat & 0x03) != 3) {
            // Not accessible during screen mode 3.
            vram[addr - 0x8000 + 0x2000 * vram_bank_num] = data;
        }
    } else if (addr < 0xFE00) {
        // If OAM DMA is currently transferring from the external bus, the write is ignored.
        if (dma_bus_block != Bus::External) {
            if (addr < 0xC000) {
                // External RAM bank.
                WriteExternalRAM(addr, data);
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
            if (!(lcd.stat & 0x02)) {
                lcd.oam[addr - 0xFE00] = data;
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
    // NR10 -- Channel 1 Sweep
    case 0xFF10:
        return audio.square1.sweep | 0x80;
    // NR11 -- Channel 1 Wave Duty & Sound Length
    case 0xFF11:
        return audio.square1.sound_length | 0x3F;
    // NR12 -- Channel 1 Volume Envelope
    case 0xFF12:
        return audio.square1.volume_envelope;
    // NR13 -- Channel 1 Low Frequency
    case 0xFF13:
        // This register is write-only.
        return 0xFF;
    // NR14 -- Channel 1 Trigger & High Frequency
    case 0xFF14:
        return audio.square1.frequency_hi | 0xBF;
    // NR21 --  Channel 2 Wave Duty & Sound Length
    case 0xFF16:
        return audio.square2.sound_length | 0x3F;
    // NR22 --  Channel 2 Volume Envelope
    case 0xFF17:
        return audio.square2.volume_envelope;
    // NR23 -- Channel 2 Low Frequency
    case 0xFF18:
        // This register is write-only.
        return 0xFF;
    // NR24 -- Channel 2 Trigger & High Frequency
    case 0xFF19:
        return audio.square2.frequency_hi | 0xBF;
    // NR30 -- Channel 3 On/Off
    case 0xFF1A:
        return audio.wave.channel_on | 0x7F;
    // NR31 -- Channel 3 Sound Length
    case 0xFF1B:
        // This register is write-only.
        return 0xFF;
    // NR32 -- Channel 3 Volume Shift
    case 0xFF1C:
        return audio.wave.volume_envelope | 0x9F;
    // NR33 -- Channel 3 Low Frequency
    case 0xFF1D:
        // This register is write-only.
        return 0xFF;
    // NR34 -- Channel 3 Trigger & High Frequency
    case 0xFF1E:
        return audio.wave.frequency_hi | 0xBF;
    // NR41 -- Channel 4 Sound Length
    case 0xFF20:
        // This register is write-only.
        return 0xFF;
    // NR42 -- Channel 4 Volume Envelope
    case 0xFF21:
        return audio.noise.volume_envelope;
    // NR43 -- Channel 4 Polynomial Counter
    case 0xFF22:
        return audio.noise.frequency_lo;
    // NR44 -- Channel 4 Trigger
    case 0xFF23:
        return audio.noise.frequency_hi | 0xBF;
    // NR50 -- Master Volume
    case 0xFF24:
        return audio.master_volume;
    // NR51 -- Sound Output Terminal Selection
    case 0xFF25:
        return audio.sound_select;
    // NR52 -- Sound On/Off
    case 0xFF26:
        return audio.ReadNR52();
    // Wave Pattern RAM
    case 0xFF30: case 0xFF31: case 0xFF32: case 0xFF33: case 0xFF34: case 0xFF35: case 0xFF36: case 0xFF37:
    case 0xFF38: case 0xFF39: case 0xFF3A: case 0xFF3B: case 0xFF3C: case 0xFF3D: case 0xFF3E: case 0xFF3F:
        if (audio.wave.channel_on) {
            // While the wave channel is enabled, reads to wave RAM return the byte containing the sample
            // currently being played.
            if (console != Console::DMG || audio.wave.reading_sample) {
                return audio.wave_ram[audio.wave.wave_pos >> 1];
            } else {
                // On DMG, the wave RAM can only be accessed within 2 cycles after the sample position has been
                // incremented, while the APU is reading the sample.
                return 0xFF;
            }
        } else {
            return audio.wave_ram[addr - 0xFF30];
        }
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
        return lcd.bg_palette_dmg;
    // OBP0 -- Sprite Palette 0 Data
    case 0xFF48:
        return lcd.obj_palette_dmg0;
    // OBP1 -- Sprite Palette 1 Data
    case 0xFF49:
        return lcd.obj_palette_dmg1;
    // WY -- Window Y Position
    case 0xFF4A:
        return lcd.window_y;
    // WX -- Window X Position
    case 0xFF4B:
        return lcd.window_x;
    // KEY1 -- Speed Switch
    case 0xFF4D:
        return speed_switch | ((game_mode == GameMode::CGB) ? 0x7E : 0xFF);
    // VBK -- VRAM bank number
    case 0xFF4F:
        if (IsConsoleCgb()) {
            // CGB in DMG mode always has bank 0 selected.
            return ((game_mode == GameMode::CGB) ? (static_cast<u8>(vram_bank_num) | 0xFE) : 0xFE);
        } else {
            return 0xFF;
        }
    // HDMA5 -- HDMA Length, Mode, and Start
    case 0xFF55:
        return ((game_mode == GameMode::CGB) ? hdma_control : 0xFF);
    // RP -- Infrared Communications
    case 0xFF56:
        return ((game_mode == GameMode::CGB) ? (infrared | 0x3C) : 0xFF);
    // BGPI -- BG Palette Index (CGB only)
    case 0xFF68:
        if (IsConsoleCgb()) {
            return lcd.bg_palette_index | 0x40;
        } else {
            return 0xFF;
        }
    // BGPD -- BG Palette Data (CGB mode only)
    case 0xFF69:
        // Palette RAM is not accessible during mode 3.
        if (game_mode == GameMode::CGB && (lcd.stat & 0x03) != 3) {
            return lcd.bg_palette_data[lcd.bg_palette_index & 0x3F];
        } else {
            return 0xFF;
        }
    // OBPI -- Sprite Palette Index (CGB only)
    case 0xFF6A:
        if (IsConsoleCgb()) {
            return lcd.obj_palette_index | 0x40;
        } else {
            return 0xFF;
        }
    // OBPD -- Sprite Palette Data (CGB mode only)
    case 0xFF6B:
        // Palette RAM is not accessible during mode 3.
        if (game_mode == GameMode::CGB && (lcd.stat & 0x03) != 3) {
            return lcd.obj_palette_data[lcd.obj_palette_index & 0x3F];
        } else {
            return 0xFF;
        }
    // SVBK -- WRAM bank number
    case 0xFF70:
        return ((game_mode == GameMode::CGB) ? (static_cast<u8>(wram_bank_num) | 0xF8) : 0xFF);
    // Undocumented
    case 0xFF6C:
        return ((game_mode == GameMode::CGB) ? (undocumented[1] | 0xFE) : 0xFF);
    case 0xFF72:
        return ((IsConsoleCgb()) ? undocumented[2] : 0xFF);
    case 0xFF73:
        return ((IsConsoleCgb()) ? undocumented[3] : 0xFF);
    case 0xFF74:
        return ((game_mode == GameMode::CGB) ? undocumented[4] : 0xFF);
    case 0xFF75:
        return ((IsConsoleCgb()) ? (undocumented[5] | 0x8F) : 0xFF);
    case 0xFF76:
    case 0xFF77:
        return ((IsConsoleCgb()) ? 0x00 : 0xFF);

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
    // NR10 -- Channel 1 Sweep
    case 0xFF10:
        if (audio.IsPoweredOn()) {
            audio.square1.sweep = data & 0x7F;
            audio.square1.SweepWriteHandler();
        }
        break;
    // NR11 -- Channel 1 Wave Pattern & Sound Length
    case 0xFF11:
        if (audio.IsPoweredOn() || IsConsoleDmg()) {
            audio.square1.sound_length = data;
            audio.square1.ReloadLengthCounter();
            audio.square1.SetDutyCycle();
        }
        break;
    // NR12 -- Channel 1 Volume Envelope
    case 0xFF12:
        if (audio.IsPoweredOn()) {
            audio.square1.volume_envelope = data;
            if ((audio.square1.volume_envelope & 0xF0) == 0) {
                audio.square1.channel_enabled = false;
            }
        }
        break;
    // NR13 -- Channel 1 Low Frequency
    case 0xFF13:
        if (audio.IsPoweredOn()) {
            audio.square1.frequency_lo = data;
        }
        break;
    // NR14 -- Channel 1 Trigger & High Frequency
    case 0xFF14:
        if (audio.IsPoweredOn()) {
            audio.square1.ExtraLengthClocking(data & 0xC7);
            audio.square1.frequency_hi = data & 0xC7;
        }
        break;
    // NR21 --  Channel 2 Wave Pattern & Sound Length
    case 0xFF16:
        if (audio.IsPoweredOn() || IsConsoleDmg()) {
            audio.square2.sound_length = data;
            audio.square2.ReloadLengthCounter();
            audio.square2.SetDutyCycle();
        }
        break;
    // NR22 --  Channel 2 Volume Envelope
    case 0xFF17:
        if (audio.IsPoweredOn()) {
            audio.square2.volume_envelope = data;
            if ((audio.square2.volume_envelope & 0xF0) == 0) {
                audio.square2.channel_enabled = false;
            }
        }
        break;
    // NR23 -- Channel 2 Low Frequency
    case 0xFF18:
        if (audio.IsPoweredOn()) {
            audio.square2.frequency_lo = data;
        }
        break;
    // NR24 -- Channel 2 Trigger & High Frequency
    case 0xFF19:
        if (audio.IsPoweredOn()) {
            audio.square2.ExtraLengthClocking(data & 0xC7);
            audio.square2.frequency_hi = data & 0xC7;
        }
        break;
    // NR30 -- Channel 3 On/Off
    case 0xFF1A:
        if (audio.IsPoweredOn()) {
            audio.wave.channel_on = data & 0x80;
            if ((audio.wave.channel_on & 0x80) == 0) {
                audio.wave.channel_enabled = false;
            }
        }
        break;
    // NR31 -- Channel 3 Sound Length
    case 0xFF1B:
        if (audio.IsPoweredOn() || IsConsoleDmg()) {
            audio.wave.sound_length = data;
            audio.wave.ReloadLengthCounter();
        }
        break;
    // NR32 -- Channel 3 Volume Shift
    case 0xFF1C:
        if (audio.IsPoweredOn()) {
            audio.wave.volume_envelope = data & 0x60;
        }
        break;
    // NR33 -- Channel 3 Low Frequency
    case 0xFF1D:
        if (audio.IsPoweredOn()) {
            audio.wave.frequency_lo = data;
        }
        break;
    // NR34 -- Channel 3 Trigger & High Frequency
    case 0xFF1E:
        if (audio.IsPoweredOn()) {
            audio.wave.ExtraLengthClocking(data & 0xC7);
            audio.wave.frequency_hi = data & 0xC7;
        }
        break;
    // NR41 -- Channel 4 Sound Length
    case 0xFF20:
        if (audio.IsPoweredOn() || IsConsoleDmg()) {
            audio.noise.sound_length = data & 0x3F;
            audio.noise.ReloadLengthCounter();
        }
        break;
    // NR42 -- Channel 4 Volume Envelope
    case 0xFF21:
        if (audio.IsPoweredOn()) {
            audio.noise.volume_envelope = data;
            if ((audio.noise.volume_envelope & 0xF0) == 0) {
                audio.noise.channel_enabled = false;
            }
        }
        break;
    // NR43 -- Channel 4 Polynomial Counter
    case 0xFF22:
        if (audio.IsPoweredOn()) {
            audio.noise.frequency_lo = data;
        }
        break;
    // NR44 -- Channel 4 Trigger
    case 0xFF23:
        if (audio.IsPoweredOn()) {
            audio.noise.ExtraLengthClocking(data & 0xC0);
            audio.noise.frequency_hi = data & 0xC0;
        }
        break;
    // NR50 -- Master Volume
    case 0xFF24:
        if (audio.IsPoweredOn()) {
            audio.master_volume = data;
        }
        break;
    // NR51 -- Sound Output Terminal Selection
    case 0xFF25:
        if (audio.IsPoweredOn()) {
            audio.sound_select = data;
        }
        break;
    // NR52 -- Sound On/Off
    case 0xFF26:
        audio.sound_on = (audio.sound_on & 0x0F) | (data & 0x80);
        break;
    // Wave Pattern RAM
    case 0xFF30: case 0xFF31: case 0xFF32: case 0xFF33: case 0xFF34: case 0xFF35: case 0xFF36: case 0xFF37:
    case 0xFF38: case 0xFF39: case 0xFF3A: case 0xFF3B: case 0xFF3C: case 0xFF3D: case 0xFF3E: case 0xFF3F:
        if (audio.wave.channel_on) {
            // While the wave channel is enabled, writes to wave RAM write the byte containing the sample
            // currently being played.
            if (console != Console::DMG || audio.wave.reading_sample) {
                // On DMG, the wave RAM can only be accessed within 2 cycles after the sample position has been
                // incremented, while the APU is reading the sample.
                audio.wave_ram[audio.wave.wave_pos >> 1] = data;
            }
        } else {
            audio.wave_ram[addr - 0xFF30] = data;
        }
        break;
    // LCDC -- LCD control
    case 0xFF40:
        lcd.lcdc = data;
        break;
    // STAT -- LCD status
    case 0xFF41:
        lcd.stat = (data & 0x78) | (lcd.stat & 0x07);
        // On DMG, if the STAT register is written during mode 1 or 0 while the LCD is on, bit 1 of the IF register
        // is set. This causes a STAT interrupt if it's enabled in IE.
        if (IsConsoleDmg() && (lcd.lcdc & 0x80) && !(lcd.stat & 0x02)) {
            lcd.SetSTATSignal();
        }
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
        oam_dma_state = DMAState::Starting;
        break;
    // BGP -- BG Palette Data
    case 0xFF47:
        lcd.bg_palette_dmg = data;
        break;
    // OBP0 -- Sprite Palette 0 Data
    case 0xFF48:
        lcd.obj_palette_dmg0 = data;
        break;
    // OBP1 -- Sprite Palette 1 Data
    case 0xFF49:
        lcd.obj_palette_dmg1 = data;
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
        speed_switch = (speed_switch & 0x80) | (data & 0x01);
        break;
    // VBK -- VRAM bank number
    case 0xFF4F:
        if (game_mode == GameMode::CGB) {
            vram_bank_num = data & 0x01;
        }
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
        if (game_mode == GameMode::CGB) {
            hdma_reg_written = true;
        }
        break;
    // RP -- Infrared Communications
    case 0xFF56:
        if (game_mode == GameMode::CGB) {
            infrared = (infrared & 0x02) | (data & 0xC1);
        }
        break;
    // BGPI -- BG Palette Index (CGB mode only)
    case 0xFF68:
        if (game_mode == GameMode::CGB) {
            lcd.bg_palette_index = data & 0xBF;
        }
        break;
    // BGPD -- BG Palette Data (CGB mode only)
    case 0xFF69:
        // Palette RAM is not accessible during mode 3.
        if (game_mode == GameMode::CGB && (lcd.stat & 0x03) != 3) {
            lcd.bg_palette_data[lcd.bg_palette_index & 0x3F] = data;
            // Increment index if auto-increment specified.
            if (lcd.bg_palette_index & 0x80) {
                lcd.bg_palette_index = (lcd.bg_palette_index + 1) & 0xBF;
            }
        }
        break;
    // OBPI -- Sprite Palette Index (CGB mode only)
    case 0xFF6A:
        if (game_mode == GameMode::CGB) {
            lcd.obj_palette_index = data & 0xBF;
        }
        break;
    // OBPD -- Sprite Palette Data (CGB mode only)
    case 0xFF6B:
        // Palette RAM is not accessible during mode 3.
        if (game_mode == GameMode::CGB && (lcd.stat & 0x03) != 3) {
            lcd.obj_palette_data[lcd.obj_palette_index & 0x3F] = data;
            // Increment index if auto-increment specified.
            if (lcd.obj_palette_index & 0x80) {
                lcd.obj_palette_index = (lcd.obj_palette_index + 1) & 0xBF;
            }
        }
        break;
    // SVBK -- WRAM bank number
    case 0xFF70:
        if (game_mode == GameMode::CGB) {
            wram_bank_num = data & 0x07;
        }
        break;
    // Undocumented
    case 0xFF6C:
        if (game_mode == GameMode::CGB) {
            undocumented[1] = data & 0x01;
        }
        break;
    case 0xFF72:
        if (IsConsoleCgb()) {
            undocumented[2] = data;
        }
        break;
    case 0xFF73:
        if (IsConsoleCgb()) {
            undocumented[3] = data;
        }
        break;
    case 0xFF74:
        if (game_mode == GameMode::CGB) {
            undocumented[4] = data;
        }
        break;
    case 0xFF75:
        if (IsConsoleCgb()) {
            undocumented[5] = data & 0x70;
        }
        break;

    default:
        break;
    }
}

} // End namespace Gb
