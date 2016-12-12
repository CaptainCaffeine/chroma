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

#include "core/memory/Memory.h"
#include "core/LCD.h"

namespace Core {

void Memory::UpdateOAM_DMA() {
    if (state_oam_dma == DMAState::RegWritten) {
        oam_transfer_addr = static_cast<u16>(oam_dma_start) << 8;
        bytes_read = 0;

        state_oam_dma = DMAState::Starting;
    } else if (state_oam_dma == DMAState::Starting) {
        // No write on the startup cycle.
        oam_transfer_byte = DMACopy(oam_transfer_addr);
        ++bytes_read;

        state_oam_dma = DMAState::Active;

        // The current OAM DMA state is not enough to determine if the external bus is currently being blocked.
        // The bus only becomes unblocked when the DMA state transitions from active to inactive. When starting 
        // a DMA while none are currently active, memory remains accessible for the two cycles when the DMA state is
        // RegWritten and Starting. But, if a DMA is started while one is already active, the state goes from
        // Active to RegWritten, without becoming Inactive, so memory remains inaccessible for those two cycles.
        dma_blocking_memory = true;
    } else if (state_oam_dma == DMAState::Active) {
        // Write the byte which was read last cycle to OAM.
        if ((lcd.stat & 0x02) != 1) {
            oam[bytes_read - 1] = oam_transfer_byte;
        } else {
            oam[bytes_read - 1] = 0xFF;
        }

        if (bytes_read == 160) {
            // Don't read on the last cycle.
            state_oam_dma = DMAState::Inactive;
            dma_blocking_memory = false;
            return;
        }

        // Read the next byte.
        oam_transfer_byte = DMACopy(oam_transfer_addr + bytes_read);
        ++bytes_read;
    }
}

u8 Memory::DMACopy(const u16 addr) const {
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
    } else if (addr < 0xF200) {
        // Echo of C000-DDFF
        return wram[addr - 0xE000 + 0x1000*((wram_bank_num == 0) ? 0 : wram_bank_num-1)];
        // For some unlicensed games and flashcarts on pre-CGB devices, reads from this region read both WRAM and
        // exernal RAM, and bitwise AND the two values together (source: AntonioND timing docs).
    } else {
        // Only 0x00-0xF1 are valid OAM DMA start addresses (several sources make that claim, at least. I've seen
        // differing ranges mentioned but this seems to work for now).
        return 0xFF;
    }
}

} // End namespace Core
