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

#include <algorithm>

#include "gb/memory/Memory.h"
#include "gb/core/GameBoy.h"
#include "gb/lcd/Lcd.h"

namespace Gb {

void Memory::UpdateOamDma() {
    if (oam_dma_state == DmaState::Starting) {
        if (bytes_read != 0) {
            oam_transfer_addr = static_cast<u16>(oam_dma_start) << 8;
            bytes_read = 0;
        } else {
            // No write on the startup cycle.
            oam_transfer_byte = DmaCopy(oam_transfer_addr);
            ++bytes_read;

            oam_dma_state = DmaState::Active;

            // The Game Boy has two major memory buses (afaik): the external bus (0x0000-0x7FFF, 0xA000-0xFDFF) and
            // the VRAM bus (0x8000-0x9FFF). I/O registers, OAM, and HRAM are all internal to the CPU. OAM DMA
            // will only block one of these buses at a time. Reads from a blocked bus will return whatever byte
            // OAM DMA read on that cycle. Writes are (probably) ignored.

            // The bus only becomes unblocked when the DMA state transitions from Active to Inactive. When starting
            // a DMA while none are currently active, memory remains accessible for the two cycles when the DMA is
            // starting. But, if a DMA is started while one is already active, the state goes from Active to Starting,
            // without becoming Inactive, so memory remains inaccessible for those two cycles.
            if (oam_transfer_addr >= 0x8000 && oam_transfer_addr < 0xA000) {
                dma_bus_block = Bus::Vram;
            } else {
                dma_bus_block = Bus::External;
            }
        }
    } else if (oam_dma_state == DmaState::Active) {
        // Write the byte which was read last cycle to OAM.
        gameboy.lcd->oam[bytes_read - 1] = oam_transfer_byte;

        if (bytes_read == 160) {
            // Don't read on the last cycle.
            oam_dma_state = DmaState::Inactive;
            dma_bus_block = Bus::None;
            return;
        }

        // Read the next byte.
        oam_transfer_byte = DmaCopy(oam_transfer_addr + bytes_read);
        ++bytes_read;
    }
}

void Memory::UpdateHdma() {
    if (hdma_reg_written) {
        if (hdma_state == DmaState::Inactive) {
            InitHdma();
        } else {
            // Can only occur when HDMA is paused.
            if (hdma_control & 0x80) {
                // Restart the copy.
                InitHdma();
            } else {
                // Stop the current copy and set bit 7 of HDMA5. Because of this, it is not possible to switch
                // directly from an HDMA to a GDMA, the current transfer must be stopped first.
                hdma_control |= 0x80;
                bytes_to_copy = 0;
                hblank_bytes = 0;
                hdma_state = DmaState::Inactive;
            }
        }

        hdma_reg_written = false;
    } else if (hdma_state == DmaState::Starting) {
        hdma_state = DmaState::Active;
    } else if (hdma_state == DmaState::Active) {
        ExecuteHdma();

        if (bytes_to_copy == 0) {
            // End the copy.
            hdma_control = 0xFF;
            hdma_state = DmaState::Inactive;
        } else if (hdma_type == HdmaType::Hdma && hblank_bytes == 0) {
            // Pause the copy until the next HBLANK.
            hdma_state = DmaState::Paused;
        }
    }
}

void Memory::InitHdma() {
    hdma_type = (hdma_control & 0x80) ? HdmaType::Hdma : HdmaType::Gdma;
    bytes_to_copy = ((hdma_control & 0x7F) + 1) * 16;
    hblank_bytes = 16;

    // If this copy was initiated without changing the source or destination addresses from the previous HDMA,
    // the copy is performed from the last addresses of the previous copy.

    hdma_control &= 0x7F;

    if (hdma_type == HdmaType::Hdma && (gameboy.lcd->stat & 0x03) != 0) {
        hdma_state = DmaState::Paused;
    } else {
        hdma_state = DmaState::Starting;
    }
}

void Memory::ExecuteHdma() {
    u16 hdma_source = (static_cast<u16>(hdma_source_hi) << 8) | hdma_source_lo;
    u16 hdma_dest = (static_cast<u16>(hdma_dest_hi | 0x80) << 8) | hdma_dest_lo;

    // The HDMA circuit always functions at a fixed speed. Every m-cycle it transfers two bytes in single speed
    // mode and one byte in double speed mode. However, if there is only one byte left to transfer (or one hblank byte
    // left for HDMA) then only one byte will be transferred in single speed mode.
    int num_bytes = std::min(2 >> double_speed, bytes_to_copy);

    if (hdma_type == HdmaType::Hdma) {
        num_bytes = std::min(num_bytes, hblank_bytes);
        hblank_bytes -= num_bytes;
    }

    bytes_to_copy -= num_bytes;

    for (int i = 0; i < num_bytes; ++i) {
        if ((gameboy.lcd->stat & 0x03) != 3) {
            vram[hdma_dest - 0x8000 + 0x2000 * vram_bank_num] = DmaCopy(hdma_source);
        }

        // Mask hdma_dest so it wraps around to the beginning of VRAM in case it increments past 0x9FFF.
        hdma_dest = (hdma_dest + 1) & 0x9FFF;
        ++hdma_source;
    }

    hdma_source_lo = hdma_source & 0x00FF;
    hdma_source_hi = hdma_source >> 8;
    hdma_dest_lo = hdma_dest & 0x00FF;
    hdma_dest_hi = (hdma_dest >> 8) & 0x1F;

    hdma_control = ((bytes_to_copy / 16) - 1) & 0x7F;
}

void Memory::SignalHdma() {
    if (hdma_state == DmaState::Paused) {
        hblank_bytes = 16;
        hdma_state = DmaState::Starting;
    }
}

u8 Memory::DmaCopy(const u16 addr) const {
    if (addr < 0x4000) {
        // ROM0 bank
        if (mbc_mode == MBC::MBC1) {
            return rom[addr + 0x4000 * ((ram_bank_num << 5) & (num_rom_banks - 1))];
        } else if (mbc_mode == MBC::MBC1M) {
            return rom[addr + 0x4000 * ((ram_bank_num << 4) & (num_rom_banks - 1))];
        } else {
            return rom[addr];
        }
    } else if (addr < 0x8000) {
        // ROM1 bank
        return rom[addr + 0x4000 * ((rom_bank_num & (num_rom_banks - 1)) - 1)];
    } else if (addr < 0xA000) {
        // VRAM -- switchable in CGB mode
        // Not accessible during screen mode 3. HDMA/GDMA cannot read VRAM.
        if ((gameboy.lcd->stat & 0x03) != 3 && hdma_state != DmaState::Active) {
            return vram[addr - 0x8000 + 0x2000 * vram_bank_num];
        } else {
            return 0xFF;
        }
    } else if (addr < 0xC000) {
        // External RAM bank.
        return ReadExternalRam(addr);
    } else if (addr < 0xD000) {
        // WRAM bank 0
        return wram[addr - 0xC000];
    } else if (addr < 0xE000) {
        // WRAM bank 1 (switchable from 1-7 in CGB mode)
        return wram[addr - 0xC000 + 0x1000 * ((wram_bank_num == 0) ? 0 : wram_bank_num - 1)];
    }

    if (hdma_state == DmaState::Active) {
        // If HDMA/GDMA attempts to read from 0xE000-0xFFFF, it will read from 0xA000-0xBFFF instead.
        return ReadExternalRam(addr - 0x4000);
    } else if (addr < 0xF000) {
        // Echo of C000-DDFF
        return wram[addr - 0xE000];
    } else if (addr < 0xF200) {
        // Echo of C000-DDFF
        return wram[addr - 0xE000 + 0x1000 * ((wram_bank_num == 0) ? 0 : wram_bank_num - 1)];
    } else {
        // Only 0x00-0xF1 are valid OAM DMA start addresses (several sources make that claim, at least. I've seen
        // differing ranges mentioned but this seems to work for now).
        return 0xFF;
    }
}

} // End namespace Gb
