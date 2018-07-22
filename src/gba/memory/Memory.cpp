// This file is a part of Chroma.
// Copyright (C) 2017-2018 Matthew Murray
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

#include <stdexcept>

#include "gba/memory/Memory.h"
#include "gba/core/Core.h"
#include "gba/cpu/Cpu.h"
#include "gba/lcd/Lcd.h"
#include "gba/lcd/Bg.h"
#include "gba/audio/Audio.h"
#include "gba/hardware/Timer.h"
#include "gba/hardware/Dma.h"
#include "gba/hardware/Keypad.h"
#include "gba/hardware/Serial.h"

namespace Gba {

Memory::Memory(const std::vector<u32>& _bios, const std::vector<u16>& _rom, const std::string& _save_path, Core& _core)
        : core(_core)
        , bios(_bios)
        , xram(xram_size / sizeof(u16))
        , iram(iram_size / sizeof(u32))
        , pram(pram_size / sizeof(u16))
        , vram(vram_size / sizeof(u16))
        , oam(oam_size / sizeof(u32))
        , rom(_rom)
        , rom_size(rom.size() * 2)
        , save_path(_save_path) {

    ReadSaveFile();
    CheckHardwareOverrides();
}

Memory::~Memory() {
    WriteSaveFile();
}

// Bus width 16.
template <>
u32 Memory::ReadRegion(const std::vector<u16>& region, const u32 region_mask, const u32 addr) const {
    // Unaligned accesses are word-aligned.
    const u32 region_addr = ((addr & region_mask) / sizeof(u16)) & ~0x1;
    return region[region_addr] | (region[region_addr + 1] << 16);
}

template <>
u16 Memory::ReadRegion(const std::vector<u16>& region, const u32 region_mask, const u32 addr) const {
    const u32 region_addr = (addr & region_mask) / sizeof(u16);
    return region[region_addr];
}

template <>
u8 Memory::ReadRegion(const std::vector<u16>& region, const u32 region_mask, const u32 addr) const {
    const u32 region_addr = (addr & region_mask) / sizeof(u16);
    return region[region_addr] >> (8 * (addr & 0x1));
}

// Bus width 32.
template <>
u32 Memory::ReadRegion(const std::vector<u32>& region, const u32 region_mask, const u32 addr) const {
    const u32 region_addr = (addr & region_mask) / sizeof(u32);
    return region[region_addr];
}

template <>
u16 Memory::ReadRegion(const std::vector<u32>& region, const u32 region_mask, const u32 addr) const {
    const u32 region_addr = (addr & region_mask) / sizeof(u32);
    return region[region_addr] >> (8 * (addr & 0x2));
}

template <>
u8 Memory::ReadRegion(const std::vector<u32>& region, const u32 region_mask, const u32 addr) const {
    const u32 region_addr = (addr & region_mask) / sizeof(u32);
    return region[region_addr] >> (8 * (addr & 0x3));
}

template <typename T>
T Memory::ReadBios(const u32 addr) const {
    // The BIOS region is not mirrored, and can only be read if the PC is currently within the BIOS.
    if (addr < bios_size) {
        if (core.cpu->GetPc() < bios_size) {
            return ReadRegion<T>(bios, bios_addr_mask, addr);
        } else {
            return core.cpu->last_bios_fetch;
        }
    } else {
        return ReadOpenBus();
    }
}

// Forward declaring template specializations of ReadIO for ReadMem.
template <> u32 Memory::ReadIO(const u32 addr) const;
template <> u16 Memory::ReadIO(const u32 addr) const;
template <> u8 Memory::ReadIO(const u32 addr) const;

template <typename T>
T Memory::ReadMem(const u32 addr, bool dma) {
    switch (GetRegion(addr)) {
    case Region::Bios:
        return ReadBios<T>(addr);
    case Region::XRam:
        return ReadXRam<T>(addr);
    case Region::IRam:
        return ReadIRam<T>(addr);
    case Region::IO:
        return ReadIO<T>(addr);
    case Region::PRam:
        return ReadPRam<T>(addr);
    case Region::VRam:
        return ReadVRam<T>(addr);
    case Region::Oam:
        return ReadOam<T>(addr);
    case Region::Rom0_l:
    case Region::Rom0_h:
    case Region::Rom1_l:
    case Region::Rom1_h:
    case Region::Rom2_l:
        return ReadRom<T>(addr);
    case Region::Eeprom:
        if (save_type == SaveType::Eeprom && EepromAddr(addr)) {
            if (dma && eeprom_ready) {
                if (eeprom_read_pos < 4) {
                    static constexpr std::array<u16, 4> eeprom_read_warmup{{0, 1, 1, 1}};
                    return eeprom_read_warmup[eeprom_read_pos++];
                } else if (eeprom_read_pos < 68) {
                    return (eeprom_read_buffer >> (eeprom_read_pos++ - 4)) & 0x1;
                } else {
                    return 0;
                }
            } else {
                return eeprom_ready;
            }
        } else {
            return ReadRom<T>(addr);
        }
    case Region::SRam_l:
    case Region::SRam_h:
        if (save_type == SaveType::Unknown) {
            InitSRam();
        }

        if (save_type == SaveType::SRam) {
            return ReadSRam<T>(addr);
        } else if (save_type == SaveType::Flash) {
            if (flash_id_mode) {
                if (addr == flash_man_addr) {
                    return chip_id & 0xFF;
                } else if (addr == flash_dev_addr) {
                    return chip_id >> 8;
                }
            }

            return ReadSRam<T>(addr);
        } else {
            // When not present, SRAM reads return either 0x00 or 0xFF. Not sure when 0xFF is returned, though.
            return 0;
        }
    default:
        return ReadOpenBus();
    }
}

template u8 Memory::ReadMem<u8>(const u32 addr, bool dma);
template u16 Memory::ReadMem<u16>(const u32 addr, bool dma);
template u32 Memory::ReadMem<u32>(const u32 addr, bool dma);

// Bus width 16.
template <>
void Memory::WriteRegion(std::vector<u16>& region, const u32 region_mask, const u32 addr, const u32 data) {
    // 32 bit writes must be aligned.
    const u32 region_addr = ((addr & region_mask) / sizeof(u16)) & ~0x1;

    region[region_addr] = data;
    region[region_addr + 1] = data >> 16;
}

template <>
void Memory::WriteRegion(std::vector<u16>& region, const u32 region_mask, const u32 addr, const u16 data) {
    const u32 region_addr = (addr & region_mask) / sizeof(u16);

    region[region_addr] = data;
}

template <>
void Memory::WriteRegion(std::vector<u16>& region, const u32 region_mask, const u32 addr, const u8 data) {
    const u32 region_addr = (addr & region_mask) / sizeof(u16);

    const u32 hi_shift = 8 * (addr & 0x1);
    region[region_addr] = (region[region_addr] & ~(0xFF << hi_shift)) | (data << hi_shift);
}

// Bus width 32.
template <>
void Memory::WriteRegion(std::vector<u32>& region, const u32 region_mask, const u32 addr, const u32 data) {
    const u32 region_addr = (addr & region_mask) / sizeof(u32);

    region[region_addr] = data;
}

template <>
void Memory::WriteRegion(std::vector<u32>& region, const u32 region_mask, const u32 addr, const u16 data) {
    const u32 region_addr = (addr & region_mask) / sizeof(u32);

    const u32 hi_shift = 8 * (addr & 0x2);
    region[region_addr] = (region[region_addr] & ~(0xFFFF << hi_shift)) | (data << hi_shift);
}

template <>
void Memory::WriteRegion(std::vector<u32>& region, const u32 region_mask, const u32 addr, const u8 data) {
    const u32 region_addr = (addr & region_mask) / sizeof(u32);

    const u32 hi_shift = 8 * (addr & 0x3);
    region[region_addr] = (region[region_addr] & ~(0xFF << hi_shift)) | (data << hi_shift);
}

template <typename T>
void Memory::WriteVRam(const u32 addr, const T data) {
    if (addr & 0x0001'0000) {
        WriteRegion(vram, vram_addr_mask2, addr, data);
    } else {
        WriteRegion(vram, vram_addr_mask1, addr, data);
        core.lcd->bg_dirty = true;
    }
}

template <typename T>
void Memory::WriteOam(const u32 addr, const T data) {
    WriteRegion(oam, oam_addr_mask, addr, data);
    core.lcd->oam_dirty = true;
}

// Specializing 8-bit writes to video memory.
// BG and Palette RAM: write the byte to both the upper and lower byte of the halfword.
// OBJ and OAM: ignore the write.
template <>
void Memory::WritePRam(const u32 addr, const u8 data) { WritePRam<u16>(addr & ~0x1, data * 0x0101); }
template <>
void Memory::WriteVRam(const u32 addr, const u8 data) {
    // TODO: The starting address of the OBJ region changes in bitmap mode.
    if ((addr & 0x0001'0000) == 0) {
        WriteVRam<u16>(addr & ~0x1, data * 0x0101);
    }
}
template <>
void Memory::WriteOam(const u32, const u8) {}

// Forward declaring template specializations of WriteIO for WriteMem.
template <> void Memory::WriteIO(const u32 addr, u32 data, u16 mask);
template <> void Memory::WriteIO(const u32 addr, u16 data, u16 mask);
template <> void Memory::WriteIO(const u32 addr, u8 data, u16 mask);

template <typename T>
void Memory::WriteMem(const u32 addr, const T data, bool dma) {
    switch (GetRegion(addr)) {
    case Region::Bios:
        // Read only.
        break;
    case Region::XRam:
        WriteXRam(addr, data);
        break;
    case Region::IRam:
        WriteIRam(addr, data);
        break;
    case Region::IO:
        WriteIO(addr, data);
        break;
    case Region::PRam:
        WritePRam(addr, data);
        break;
    case Region::VRam:
        WriteVRam(addr, data);
        break;
    case Region::Oam:
        WriteOam(addr, data);
        break;
    case Region::Rom0_l:
    case Region::Rom0_h:
    case Region::Rom1_l:
    case Region::Rom1_h:
    case Region::Rom2_l:
        // Read only.
        break;
    case Region::Eeprom:
        if (dma && EepromAddr(addr)) {
            if (save_type == SaveType::Unknown) {
                save_type = SaveType::Eeprom;
            }

            if (save_type == SaveType::Eeprom && eeprom_ready) {
                eeprom_bitstream.push_back(data & 0x1);
            }
        }
        break;
    case Region::SRam_l:
    case Region::SRam_h:
        if (save_type == SaveType::Unknown) {
            if (addr == flash_cmd_addr1 && data == FlashCmd::Start1) {
                InitFlash();
            } else {
                InitSRam();
            }
        }

        if (save_type == SaveType::SRam) {
            WriteSRam(addr, data);
        } else if (save_type == SaveType::Flash) {
            WriteFlash(addr, data);
        }
        break;
    default:
        break;
    }
}

template void Memory::WriteMem<u8>(const u32 addr, const u8 data, bool dma);
template void Memory::WriteMem<u16>(const u32 addr, const u16 data, bool dma);
template void Memory::WriteMem<u32>(const u32 addr, const u32 data, bool dma);

template <typename T>
int Memory::AccessTime(const u32 addr, AccessType access_type, bool force_sequential) {
    constexpr int u32_access = sizeof(T) / 4;
    const bool sequential = force_sequential || (addr - last_addr) <= 4;
    last_addr = addr;

    const auto RomTime = [this, sequential, access_type](int i) -> int {
        int access_cycles;
        if (sequential) {
            access_cycles = wait_state_s[i] << u32_access;
        } else {
            access_cycles = wait_state_n[i] + wait_state_s[i] * u32_access;
        }

        if (PrefetchEnabled() && access_type == AccessType::Opcode) {
            if (prefetched_opcodes > 0) {
                prefetched_opcodes -= 1;
                return 1 << u32_access;
            } else {
                int free_cycles = std::min(access_cycles - (1 << u32_access), prefetch_cycles);
                access_cycles -= free_cycles;
                prefetch_cycles -= free_cycles;
            }
        }

        return access_cycles;
    };

    int access_cycles;
    switch (GetRegion(addr)) {
    case Region::Bios:
        access_cycles = 1;
        break;
    case Region::XRam:
        access_cycles = 3 << u32_access;
        break;
    case Region::IRam:
        access_cycles = 1;
        break;
    case Region::IO:
        // Despite being 16 bits wide, 32-bit accesses to IO registers do not incur an extra wait state.
        // Apparently the 16-bit registers are packaged together in pairs.
        access_cycles = 1;
        break;
    case Region::PRam:
        access_cycles = 1 << u32_access;
        break;
    case Region::VRam:
        access_cycles = 1 << u32_access;
        break;
    case Region::Oam:
        access_cycles = 1;
        break;
    case Region::Rom0_l:
    case Region::Rom0_h:
        access_cycles = RomTime(0);
        break;
    case Region::Rom1_l:
    case Region::Rom1_h:
        access_cycles = RomTime(1);
        break;
    case Region::Rom2_l:
    case Region::Eeprom:
        access_cycles = RomTime(2);
        break;
    case Region::SRam_l:
    case Region::SRam_h:
        access_cycles = wait_state_sram;
        break;
    default:
        access_cycles = 1;
        break;
    }

    if (PrefetchEnabled() && core.cpu->GetPc() >= BaseAddr::Rom
                          && (addr < BaseAddr::Rom || addr >= BaseAddr::Max)
                          && access_type == AccessType::Normal) {
        RunPrefetch(access_cycles);
    }

    return access_cycles;
}

template int Memory::AccessTime<u8>(const u32 addr, AccessType access_type, bool force_sequential);
template int Memory::AccessTime<u16>(const u32 addr, AccessType access_type, bool force_sequential);
template int Memory::AccessTime<u32>(const u32 addr, AccessType access_type, bool force_sequential);

void Memory::UpdateWaitStates() {
    auto WaitStates = [this](int shift) {
        u16 mask = 0x3 << shift;
        if ((waitcnt & mask) == mask) {
            return 8;
        } else {
            return 4 - ((waitcnt & mask) >> shift);
        }
    };

    wait_state_sram = 1 + WaitStates(0);
    wait_state_n[0] = 1 + WaitStates(2);
    wait_state_s[0] = 1 + ((waitcnt & 0x010) ? 1 : 2);
    wait_state_n[1] = 1 + WaitStates(5);
    wait_state_s[1] = 1 + ((waitcnt & 0x080) ? 1 : 4);
    wait_state_n[2] = 1 + WaitStates(8);
    wait_state_s[2] = 1 + ((waitcnt & 0x400) ? 1 : 8);
}

void Memory::RunPrefetch(int cycles) {
    prefetch_cycles += cycles;

    int wait_states;
    switch (GetRegion(core.cpu->GetPc())) {
    case Region::Rom0_l:
    case Region::Rom0_h:
        wait_states = wait_state_s[0];
        break;
    case Region::Rom1_l:
    case Region::Rom1_h:
        wait_states = wait_state_s[1];
        break;
    case Region::Rom2_l:
    case Region::Eeprom:
        wait_states = wait_state_s[2];
        break;
    default:
        throw std::runtime_error("Ran prefetch while the PC is not in ROM.");
        break;
    }

    wait_states -= 1;

    if (core.cpu->ThumbMode()) {
        if (prefetch_cycles >= wait_states) {
            prefetched_opcodes = std::min(prefetched_opcodes + 1, 8);
            prefetch_cycles -= wait_states;
        }
    } else {
        wait_states *= 2;
        if (prefetch_cycles >= wait_states) {
            prefetched_opcodes = std::min(prefetched_opcodes + 1, 4);
            prefetch_cycles -= wait_states;
        }
    }
}

template <>
u32 Memory::ReadIO(const u32 addr) const {
    // Unaligned accesses are word-aligned.
    return ReadIO<u16>(addr & ~0x3) | (ReadIO<u16>((addr & ~0x3) + 2) << 16);
}

template <>
u8 Memory::ReadIO(const u32 addr) const {
    return ReadIO<u16>(addr) >> (8 * (addr & 0x1));
}

template <>
void Memory::WriteIO(const u32 addr, const u32 data, const u16) {
    // 32 bit writes must be aligned.
    WriteIO<u16>(addr & ~0x3, data);
    WriteIO<u16>((addr & ~0x3) + 2, data >> 16);
}

template <>
void Memory::WriteIO(const u32 addr, const u8 data, const u16) {
    const u32 hi_shift = 8 * (addr & 0x1);
    WriteIO<u16>(addr, data << hi_shift, 0xFF << hi_shift);
}

template <>
u16 Memory::ReadIO(const u32 addr) const {
    switch (addr & ~0x1) {
    case DISPCNT:
        return core.lcd->control.Read();
    case GREENSWAP:
        return core.lcd->green_swap.Read();
    case DISPSTAT:
        return core.lcd->status.Read();
    case VCOUNT:
        return core.lcd->vcount.Read();
    case BG0CNT:
        return core.lcd->bgs[0].control.Read();
    case BG1CNT:
        return core.lcd->bgs[1].control.Read();
    case BG2CNT:
        return core.lcd->bgs[2].control.Read();
    case BG3CNT:
        return core.lcd->bgs[3].control.Read();
    case WININ:
        return core.lcd->winin.Read();
    case WINOUT:
        return core.lcd->winout.Read();
    case BLDCNT:
        return core.lcd->blend_control.Read();
    case BLDALPHA:
        return core.lcd->blend_alpha.Read();
    case SOUNDCNT_L:
        return core.audio->psg_control.Read();
    case SOUNDCNT_H:
        return core.audio->fifo_control.Read();
    case SOUNDCNT_X:
        return core.audio->sound_on.Read();
    case SOUNDBIAS:
        return core.audio->soundbias.Read();
    case DMA0CNT_L:
        return 0x0000;
    case DMA0CNT_H:
        return core.dma[0].control.Read();
    case DMA1CNT_L:
        return 0x0000;
    case DMA1CNT_H:
        return core.dma[1].control.Read();
    case DMA2CNT_L:
        return 0x0000;
    case DMA2CNT_H:
        return core.dma[2].control.Read();
    case DMA3CNT_L:
        return 0x0000;
    case DMA3CNT_H:
        return core.dma[3].control.Read();
    case TM0CNT_L:
        return core.timers[0].counter.Read();
    case TM0CNT_H:
        return core.timers[0].control.Read();
    case TM1CNT_L:
        return core.timers[1].counter.Read();
    case TM1CNT_H:
        return core.timers[1].control.Read();
    case TM2CNT_L:
        return core.timers[2].counter.Read();
    case TM2CNT_H:
        return core.timers[2].control.Read();
    case TM3CNT_L:
        return core.timers[3].counter.Read();
    case TM3CNT_H:
        return core.timers[3].control.Read();
    case SIOMULTI0:
        return core.serial->data0.Read();
    case SIOMULTI1:
        return core.serial->data1.Read();
    case SIOMULTI2:
        return core.serial->data2.Read();
    case SIOMULTI3:
        return core.serial->data3.Read();
    case SIOCNT:
        return core.serial->control.Read();
    case SIOMLTSEND:
        return core.serial->send.Read();
    case KEYINPUT:
        return core.keypad->input.Read();
    case KEYCNT:
        return core.keypad->control.Read();
    case RCNT:
        return core.serial->mode.Read();
    case JOYCNT:
        return core.serial->joybus_control.Read();
    case JOYRECV_L:
        core.serial->joybus_status &= ~Serial::joystat_recv;
        return core.serial->joybus_recv_l.Read();
    case JOYRECV_H:
        core.serial->joybus_status &= ~Serial::joystat_recv;
        return core.serial->joybus_recv_h.Read();
    case JOYTRANS_L:
        return core.serial->joybus_trans_l.Read();
    case JOYTRANS_H:
        return core.serial->joybus_trans_h.Read();
    case JOYSTAT:
        return core.serial->joybus_status.Read();
    case IE:
        return intr_enable.Read();
    case IF:
        return intr_flags.Read();
    case WAITCNT:
        return waitcnt.Read();
    case IME:
        return master_enable.Read();
    case HALTCNT:
        return haltcnt.Read();
    default:
        return ReadOpenBus();
    }
}

template <>
void Memory::WriteIO(const u32 addr, const u16 data, const u16 mask) {
    switch (addr & ~0x1) {
    case DISPCNT:
        core.lcd->WriteControl(data, mask);
        break;
    case GREENSWAP:
        core.lcd->green_swap.Write(data, mask);
        break;
    case DISPSTAT:
        core.lcd->status.Write(data, mask);
        break;
    case BG0CNT:
        core.lcd->bgs[0].control.Write(data, mask);
        core.lcd->bgs[0].dirty = true;
        break;
    case BG1CNT:
        core.lcd->bgs[1].control.Write(data, mask);
        core.lcd->bgs[1].dirty = true;
        break;
    case BG2CNT:
        core.lcd->bgs[2].control.Write(data, mask);
        core.lcd->bgs[2].dirty = true;
        break;
    case BG3CNT:
        core.lcd->bgs[3].control.Write(data, mask);
        core.lcd->bgs[3].dirty = true;
        break;
    case BG0HOFS:
        core.lcd->bgs[0].scroll_x.Write(data, mask);
        break;
    case BG0VOFS:
        core.lcd->bgs[0].scroll_y.Write(data, mask);
        break;
    case BG1HOFS:
        core.lcd->bgs[1].scroll_x.Write(data, mask);
        break;
    case BG1VOFS:
        core.lcd->bgs[1].scroll_y.Write(data, mask);
        break;
    case BG2HOFS:
        core.lcd->bgs[2].scroll_x.Write(data, mask);
        break;
    case BG2VOFS:
        core.lcd->bgs[2].scroll_y.Write(data, mask);
        break;
    case BG3HOFS:
        core.lcd->bgs[3].scroll_x.Write(data, mask);
        break;
    case BG3VOFS:
        core.lcd->bgs[3].scroll_y.Write(data, mask);
        break;
    case BG2PA:
        core.lcd->bgs[2].affine_a.Write(data, mask);
        break;
    case BG2PB:
        core.lcd->bgs[2].affine_b.Write(data, mask);
        break;
    case BG2PC:
        core.lcd->bgs[2].affine_c.Write(data, mask);
        break;
    case BG2PD:
        core.lcd->bgs[2].affine_d.Write(data, mask);
        break;
    case BG2X_L:
        core.lcd->bgs[2].offset_x_l.Write(data, mask);
        core.lcd->bgs[2].LatchReferencePointX();
        break;
    case BG2X_H:
        core.lcd->bgs[2].offset_x_h.Write(data, mask);
        core.lcd->bgs[2].LatchReferencePointX();
        break;
    case BG2Y_L:
        core.lcd->bgs[2].offset_y_l.Write(data, mask);
        core.lcd->bgs[2].LatchReferencePointY();
        break;
    case BG2Y_H:
        core.lcd->bgs[2].offset_y_h.Write(data, mask);
        core.lcd->bgs[2].LatchReferencePointY();
        break;
    case BG3PA:
        core.lcd->bgs[3].affine_a.Write(data, mask);
        break;
    case BG3PB:
        core.lcd->bgs[3].affine_b.Write(data, mask);
        break;
    case BG3PC:
        core.lcd->bgs[3].affine_c.Write(data, mask);
        break;
    case BG3PD:
        core.lcd->bgs[3].affine_d.Write(data, mask);
        break;
    case BG3X_L:
        core.lcd->bgs[3].offset_x_l.Write(data, mask);
        core.lcd->bgs[3].LatchReferencePointX();
        break;
    case BG3X_H:
        core.lcd->bgs[3].offset_x_h.Write(data, mask);
        core.lcd->bgs[3].LatchReferencePointX();
        break;
    case BG3Y_L:
        core.lcd->bgs[3].offset_y_l.Write(data, mask);
        core.lcd->bgs[3].LatchReferencePointY();
        break;
    case BG3Y_H:
        core.lcd->bgs[3].offset_y_h.Write(data, mask);
        core.lcd->bgs[3].LatchReferencePointY();
        break;
    case WIN0H:
        core.lcd->windows[0].width.Write(data, mask);
        break;
    case WIN1H:
        core.lcd->windows[1].width.Write(data, mask);
        break;
    case WIN0V:
        core.lcd->windows[0].height.Write(data, mask);
        break;
    case WIN1V:
        core.lcd->windows[1].height.Write(data, mask);
        break;
    case WININ:
        core.lcd->winin.Write(data, mask);
        break;
    case WINOUT:
        core.lcd->winout.Write(data, mask);
        break;
    case MOSAIC:
        core.lcd->mosaic.Write(data, mask);
        break;
    case BLDCNT:
        core.lcd->blend_control.Write(data, mask);
        break;
    case BLDALPHA:
        core.lcd->blend_alpha.Write(data, mask);
        break;
    case BLDY:
        core.lcd->blend_fade.Write(data, mask);
        break;
    case SOUNDCNT_L:
        core.audio->psg_control.Write(data, mask);
        break;
    case SOUNDCNT_H:
        core.audio->WriteFifoControl(data, mask);
        break;
    case SOUNDCNT_X:
        core.audio->sound_on.Write(data, mask);
        break;
    case SOUNDBIAS:
        core.audio->soundbias.Write(data, mask);
        break;
    case FIFO_A_L:
    case FIFO_A_H:
        core.audio->fifos[0].Write(data, mask);
        break;
    case FIFO_B_L:
    case FIFO_B_H:
        core.audio->fifos[1].Write(data, mask);
        break;
    case DMA0SAD_L:
        core.dma[0].source_l.Write(data, mask);
        break;
    case DMA0SAD_H:
        core.dma[0].source_h.Write(data, mask);
        break;
    case DMA0DAD_L:
        core.dma[0].dest_l.Write(data, mask);
        break;
    case DMA0DAD_H:
        core.dma[0].dest_h.Write(data, mask);
        break;
    case DMA0CNT_L:
        core.dma[0].word_count.Write(data, mask);
        break;
    case DMA0CNT_H:
        core.dma[0].WriteControl(data, mask);
        break;
    case DMA1SAD_L:
        core.dma[1].source_l.Write(data, mask);
        break;
    case DMA1SAD_H:
        core.dma[1].source_h.Write(data, mask);
        break;
    case DMA1DAD_L:
        core.dma[1].dest_l.Write(data, mask);
        break;
    case DMA1DAD_H:
        core.dma[1].dest_h.Write(data, mask);
        break;
    case DMA1CNT_L:
        core.dma[1].word_count.Write(data, mask);
        break;
    case DMA1CNT_H:
        core.dma[1].WriteControl(data, mask);
        break;
    case DMA2SAD_L:
        core.dma[2].source_l.Write(data, mask);
        break;
    case DMA2SAD_H:
        core.dma[2].source_h.Write(data, mask);
        break;
    case DMA2DAD_L:
        core.dma[2].dest_l.Write(data, mask);
        break;
    case DMA2DAD_H:
        core.dma[2].dest_h.Write(data, mask);
        break;
    case DMA2CNT_L:
        core.dma[2].word_count.Write(data, mask);
        break;
    case DMA2CNT_H:
        core.dma[2].WriteControl(data, mask);
        break;
    case DMA3SAD_L:
        core.dma[3].source_l.Write(data, mask);
        break;
    case DMA3SAD_H:
        core.dma[3].source_h.Write(data, mask);
        break;
    case DMA3DAD_L:
        core.dma[3].dest_l.Write(data, mask);
        break;
    case DMA3DAD_H:
        core.dma[3].dest_h.Write(data, mask);
        break;
    case DMA3CNT_L:
        core.dma[3].word_count.Write(data, mask);
        break;
    case DMA3CNT_H:
        core.dma[3].WriteControl(data, mask);
        break;
    case TM0CNT_L:
        core.timers[0].reload.Write(data, mask);
        break;
    case TM0CNT_H:
        core.timers[0].WriteControl(data, mask);
        break;
    case TM1CNT_L:
        core.timers[1].reload.Write(data, mask);
        break;
    case TM1CNT_H:
        core.timers[1].WriteControl(data, mask);
        break;
    case TM2CNT_L:
        core.timers[2].reload.Write(data, mask);
        break;
    case TM2CNT_H:
        core.timers[2].WriteControl(data, mask);
        break;
    case TM3CNT_L:
        core.timers[3].reload.Write(data, mask);
        break;
    case TM3CNT_H:
        core.timers[3].WriteControl(data, mask);
        break;
    case SIOMULTI0:
        core.serial->data0.Write(data, mask);
        break;
    case SIOMULTI1:
        core.serial->data1.Write(data, mask);
        break;
    case SIOMULTI2:
        core.serial->data2.Write(data, mask);
        break;
    case SIOMULTI3:
        core.serial->data3.Write(data, mask);
        break;
    case SIOCNT:
        core.serial->control.Write(data, mask);
        break;
    case SIOMLTSEND:
        core.serial->send.Write(data, mask);
        break;
    case KEYCNT:
        core.keypad->control.Write(data, mask);
        break;
    case RCNT:
        core.serial->mode.Write(data, mask);
        break;
    case JOYCNT:
        // Bits 0-2 of JOYCNT behave like IF. The IRQ enable bit is normally writeable.
        core.serial->joybus_control.Clear(data & Serial::joycnt_ack_mask);
        core.serial->joybus_control.Write(data & Serial::joycnt_irq_enable, mask);
        break;
    case JOYRECV_L:
        core.serial->joybus_recv_l.Write(data, mask);
        break;
    case JOYRECV_H:
        core.serial->joybus_recv_h.Write(data, mask);
        break;
    case JOYTRANS_L:
        core.serial->joybus_trans_l.Write(data, mask);
        core.serial->joybus_status |= Serial::joystat_trans;
        break;
    case JOYTRANS_H:
        core.serial->joybus_trans_h.Write(data, mask);
        core.serial->joybus_status |= Serial::joystat_trans;
        break;
    case JOYSTAT:
        core.serial->joybus_status.Write(data, mask);
        break;
    case IE:
        intr_enable.Write(data, mask);
        break;
    case IF:
        // Writing "1" to a bit in IF clears that bit.
        intr_flags.Clear(data);
        break;
    case WAITCNT:
        waitcnt.Write(data, mask);
        UpdateWaitStates();
        break;
    case IME:
        master_enable.Write(data, mask);
        break;
    case HALTCNT:
        haltcnt.Write(data, mask);
        if ((mask & 0xFF00) == 0xFF00 && (data & 0x8000) == 0) {
            if (master_enable == 0 && intr_enable == 0) {
                throw std::runtime_error("The CPU has hung: halt mode entered with interrupts disabled.");
            }

            core.cpu->Halt();
        }
        break;
    default:
        break;
    }
}

u32 Memory::ReadOpenBus() const {
    if (core.cpu->ArmMode()) {
        return core.cpu->GetPrefetchedOpcode(2);
    }

    switch (GetRegion(core.cpu->GetPc())) {
    case Region::Bios:
        if ((core.cpu->GetPc() & 0x3) == 0) {
            return bios[core.cpu->GetPc() / 4];
        } else {
            return core.cpu->GetPrefetchedOpcode(1) | (core.cpu->GetPrefetchedOpcode(2) << 16);
        }
    case Region::Oam:
        if ((core.cpu->GetPc() & 0x3) == 0) {
            return oam[core.cpu->GetPc() / 4];
        } else {
            return core.cpu->GetPrefetchedOpcode(1) | (core.cpu->GetPrefetchedOpcode(2) << 16);
        }
    case Region::XRam:
    case Region::PRam:
    case Region::VRam:
    case Region::Rom0_l:
    case Region::Rom0_h:
    case Region::Rom1_l:
    case Region::Rom1_h:
    case Region::Rom2_l:
    case Region::Eeprom:
        return core.cpu->GetPrefetchedOpcode(2) | (core.cpu->GetPrefetchedOpcode(2) << 16);
    case Region::IRam:
        if ((core.cpu->GetPc() & 0x3) == 0) {
            return core.cpu->GetPrefetchedOpcode(2) | (core.cpu->GetPrefetchedOpcode(1) << 16);
        } else {
            return core.cpu->GetPrefetchedOpcode(1) | (core.cpu->GetPrefetchedOpcode(2) << 16);
        }
    case Region::IO:
    case Region::SRam_l:
    case Region::SRam_h:
    default:
        // Executing code from these regions is not strictly forbidden, but will likely go poorly. I don't know
        // what open-bus reads will return.
        return 0;
    }
}

} // End namespace Gba
