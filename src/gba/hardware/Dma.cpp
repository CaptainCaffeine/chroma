// This file is a part of Chroma.
// Copyright (C) 2018 Matthew Murray
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

#include "gba/hardware/Dma.h"
#include "gba/core/Core.h"
#include "gba/memory/Memory.h"
#include "gba/cpu/Cpu.h"

namespace Gba {

Dma::Dma(int _id, Core& _core)
        : id(_id)
        , core(_core) {
    if (id == 0) {
        // The source address for DMA0 must be within internal memory.
        source_h.write_mask = 0x07FF;
    } else if (id == 3) {
        // The destination address for DMA3 can be in any memory.
        dest_h.write_mask = 0x0FFF;

        // The word count is 16 bits instead of 14.
        word_count.write_mask = 0xFFFF;

        // The DRQ bit is accessible.
        control.read_mask = 0xFFE0;
        control.write_mask = 0xFFE0;
    }
}

void Dma::WriteControl(const u16 data, const u16 mask) {
    bool was_enabled = control & enable;
    control.Write(data, mask);

    if (!was_enabled && (control & enable)) {
        source = source_l | (source_h << 16);
        dest = dest_l | (dest_h << 16);

        bad_source = source < BaseAddr::XRam || source >= BaseAddr::Max || (id == 0 && source >= BaseAddr::Rom);

        if (TransferWidth() == 4) {
            // Both addresses must be word-aligned.
            source &= ~0x3;
            dest &= ~0x3;
        }

        ReloadWordCount();

        if (StartTiming() != Immediate) {
            // If DMA0 is initialized with Special start timing, it will never get triggered.
            paused = true;
        } else {
            paused = false;
            core.cpu->dma_active = true;
        }
    }
}

void Dma::Trigger(DmaTiming event) {
    if ((control & enable) && StartTiming() == event) {
        paused = false;
        core.cpu->dma_active = true;
    }
}

int Dma::Run() {
    int cycles_taken = 0;

    if (starting) {
        // Two I-cycles to start the transfer.
        cycles_taken = 2;

        // First read & write accesses are non-sequential, all subsequent accesses are sequential.
        if (TransferWidth() == 2) {
            cycles_taken += Transfer<u16>(AccessType::Normal);
        } else {
            cycles_taken += Transfer<u32>(AccessType::Normal);
        }

        starting = false;
    } else {
        if (TransferWidth() == 2) {
            cycles_taken += Transfer<u16>(AccessType::Sequential);
        } else {
            cycles_taken += Transfer<u32>(AccessType::Sequential);
        }
    }

    if (--remaining_chunks == 0) {
        // The transfer has finished.
        if (control & irq_enable) {
            core.mem->RequestInterrupt(Interrupt::Dma0 << id);
        }

        if (DestControl() == Reload) {
            dest = dest_l | (dest_h << 16);
            if (TransferWidth() == 4) {
                dest &= ~0x3;
            }
        }

        if ((control & repeat) && StartTiming() != Immediate) {
            // If repeat is enabled, reload the chunk count and wait for the next DMA trigger event.
            ReloadWordCount();
            paused = true;
        } else {
            // Disable the DMA.
            control &= ~enable;
        }

        if (id == 3 && dest >= BaseAddr::Eeprom && core.mem->EepromAddr(dest)) {
            core.mem->ParseEepromCommand();
        }

        // Check if there are any other currently active DMA transfers.
        core.cpu->dma_active = std::any_of(core.dma.cbegin(), core.dma.cend(), [](const auto& dma) {
            return dma.Active();
        });
    }

    return cycles_taken;
}

template<typename T>
int Dma::Transfer(AccessType sequential) {
    if (!bad_source) {
        core.mem->transfer_reg = core.mem->ReadMem<T>(source, true);
        if (sizeof(T) == sizeof(u16)) {
            core.mem->transfer_reg |= core.mem->transfer_reg << 16;
        }
    }
    core.mem->WriteMem<T>(dest, core.mem->transfer_reg, true);

    int cycles = core.mem->AccessTime<T>(source, sequential) + core.mem->AccessTime<T>(dest, sequential);

    if (source >= BaseAddr::Rom && source < BaseAddr::SRam) {
        // Sequential accesses to ROM always read from the address incrementer.
        source += TransferWidth();
    } else {
        switch (SourceControl()) {
        case Increment:
            source += TransferWidth();
            break;
        case Decrement:
            source -= TransferWidth();
            break;
        default:
            break;
        }
    }

    switch (DestControl()) {
    case Increment:
    case Reload:
        dest += TransferWidth();
        break;
    case Decrement:
        dest -= TransferWidth();
        break;
    default:
        break;
    }

    return cycles;
}

void Dma::ReloadWordCount() {
    if (word_count != 0) {
        remaining_chunks = word_count;
    } else {
        // If the word count register contains 0, the actual count is 0x1'0000 for DMA3 and 0x4000 otherwise.
        remaining_chunks = word_count.write_mask + 1;
    }

    starting = true;
}

} // End namespace Gba
