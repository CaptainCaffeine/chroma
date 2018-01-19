// This file is a part of Chroma.
// Copyright (C) 2017 Matthew Murray
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

#include "common/CommonFuncs.h"
#include "gba/memory/Memory.h"
#include "gba/cpu/Cpu.h"

namespace Gba {

Memory::Memory(const std::vector<u32>& _bios, const std::vector<u16>& _rom)
        : bios(_bios)
        , xram(xram_size / sizeof(u16))
        , iram(iram_size / sizeof(u32))
        , pram(pram_size / sizeof(u16))
        , vram(vram_size / sizeof(u16))
        , oam(oam_size / sizeof(u32))
        , rom(_rom) {}

// Bus width 16.
template <>
u32 Memory::ReadRegion(const std::vector<u16>& region, const AddressMask region_mask, const u32 addr) const {
    const u32 region_addr = (addr & region_mask) / sizeof(u16);

    // Unaligned accesses are rotated.
    return RotateRight(region[region_addr] | (region[region_addr + 1] << 16), (addr & 0x3) * 8);
}

template <>
u16 Memory::ReadRegion(const std::vector<u16>& region, const AddressMask region_mask, const u32 addr) const {
    const u32 region_addr = (addr & region_mask) / sizeof(u16);

    // Unaligned accesses are rotated.
    return RotateRight(region[region_addr], (addr & 0x1) * 8);
}

template <>
u8 Memory::ReadRegion(const std::vector<u16>& region, const AddressMask region_mask, const u32 addr) const {
    const u32 region_addr = (addr & region_mask) / sizeof(u16);
    return region[region_addr] >> (8 * (addr & 0x1));
}

// Bus width 32.
template <>
u32 Memory::ReadRegion(const std::vector<u32>& region, const AddressMask region_mask, const u32 addr) const {
    const u32 region_addr = (addr & region_mask) / sizeof(u32);

    // Unaligned accesses are rotated.
    return RotateRight(region[region_addr], (addr & 0x3) * 8);
}

template <>
u16 Memory::ReadRegion(const std::vector<u32>& region, const AddressMask region_mask, const u32 addr) const {
    const u32 region_addr = (addr & region_mask) / sizeof(u32);

    // Unaligned accesses are rotated.
    return RotateRight(region[region_addr] >> (8 * (addr & 0x2)), (addr & 0x1) * 8);
}

template <>
u8 Memory::ReadRegion(const std::vector<u32>& region, const AddressMask region_mask, const u32 addr) const {
    const u32 region_addr = (addr & region_mask) / sizeof(u32);
    return region[region_addr] >> (8 * (addr & 0x3));
}

// Forward declaring template specializations of ReadIO for ReadMem.
template <> u32 Memory::ReadIO(const u32 addr) const;
template <> u16 Memory::ReadIO(const u32 addr) const;
template <> u8 Memory::ReadIO(const u32 addr) const;

template <typename T>
T Memory::ReadMem(const u32 addr) const {
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
    case Region::Rom0:
    case Region::Rom1:
    case Region::Rom2:
        return ReadRom<T>(addr);
    case Region::SRam:
        return 0;
    default:
        return 0;
    }
}

template u8 Memory::ReadMem<u8>(const u32 addr) const;
template u16 Memory::ReadMem<u16>(const u32 addr) const;
template u32 Memory::ReadMem<u32>(const u32 addr) const;

// Bus width 16.
template <>
void Memory::WriteRegion(std::vector<u16>& region, const AddressMask region_mask, const u32 addr, const u32 data) {
    // 32 bit writes must be aligned.
    const u32 region_addr = ((addr & region_mask) / sizeof(u16)) & ~0x1;

    region[region_addr] = data;
    region[region_addr + 1] = data >> 16;
}

template <>
void Memory::WriteRegion(std::vector<u16>& region, const AddressMask region_mask, const u32 addr, const u16 data) {
    const u32 region_addr = (addr & region_mask) / sizeof(u16);

    region[region_addr] = data;
}

template <>
void Memory::WriteRegion(std::vector<u16>& region, const AddressMask region_mask, const u32 addr, const u8 data) {
    const u32 region_addr = (addr & region_mask) / sizeof(u16);

    const u32 hi_shift = 8 * (addr & 0x1);
    region[region_addr] = (region[region_addr] & ~(0xFF << hi_shift)) | (data << hi_shift);
}

// Bus width 32.
template <>
void Memory::WriteRegion(std::vector<u32>& region, const AddressMask region_mask, const u32 addr, const u32 data) {
    const u32 region_addr = (addr & region_mask) / sizeof(u32);

    region[region_addr] = data;
}

template <>
void Memory::WriteRegion(std::vector<u32>& region, const AddressMask region_mask, const u32 addr, const u16 data) {
    const u32 region_addr = (addr & region_mask) / sizeof(u32);

    const u32 hi_shift = 8 * (addr & 0x2);
    region[region_addr] = (region[region_addr] & ~(0xFFFF << hi_shift)) | (data << hi_shift);
}

template <>
void Memory::WriteRegion(std::vector<u32>& region, const AddressMask region_mask, const u32 addr, const u8 data) {
    const u32 region_addr = (addr & region_mask) / sizeof(u32);

    const u32 hi_shift = 8 * (addr & 0x3);
    region[region_addr] = (region[region_addr] & ~(0xFF << hi_shift)) | (data << hi_shift);
}

// Forward declaring template specializations of WriteIO for WriteMem.
template <> void Memory::WriteIO(const u32 addr, u32 data, u16 mask);
template <> void Memory::WriteIO(const u32 addr, u16 data, u16 mask);
template <> void Memory::WriteIO(const u32 addr, u8 data, u16 mask);

template <typename T>
void Memory::WriteMem(const u32 addr, const T data) {
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
    case Region::Rom0:
    case Region::Rom1:
    case Region::Rom2:
        // Read only.
        break;
    case Region::SRam:
        break;
    default:
        break;
    }
}

template void Memory::WriteMem<u8>(const u32 addr, const u8 data);
template void Memory::WriteMem<u16>(const u32 addr, const u16 data);
template void Memory::WriteMem<u32>(const u32 addr, const u32 data);

template <typename T>
T Memory::ReadBios(const u32 addr) const {
    // The BIOS region is not mirrored, and can only be read if the PC is currently within the BIOS.
    if (addr < bios_size && cpu->GetPc() < bios_size) {
        return ReadRegion<T>(bios, bios_addr_mask, addr);
    } else {
        return 0;
    }
}

template u8 Memory::ReadBios<u8>(const u32 addr) const;
template u16 Memory::ReadBios<u16>(const u32 addr) const;
template u32 Memory::ReadBios<u32>(const u32 addr) const;

template <typename T>
int Memory::AccessTime(const u32 addr) {
    int u32_access = sizeof(T) / 4;
    bool sequential = (addr - last_addr) <= 4;
    last_addr = addr;

    auto RomTime = [this, u32_access, sequential](int i) -> int {
        if (sequential) {
            return wait_state_s[i] << u32_access;
        } else {
            return wait_state_n[i] + wait_state_s[i] * u32_access;
        }
    };

    switch (GetRegion(addr)) {
    case Region::Bios:
        return 1;
    case Region::XRam:
        return 3 << u32_access;
    case Region::IRam:
        return 1;
    case Region::IO:
        // Despite being 16 bits wide, 32-bit accesses to IO registers do not incur an extra wait state.
        // Apparently the 16-bit registers are packaged together in pairs.
        return 1;
    case Region::PRam:
        return 1 << u32_access;
    case Region::VRam:
        return 1 << u32_access;
    case Region::Oam:
        return 1;
    case Region::Rom0:
        return RomTime(0);
    case Region::Rom1:
        return RomTime(1);
    case Region::Rom2:
        return RomTime(2);
    case Region::SRam:
        return wait_state_sram;
    default:
        return 1;
    }
}

template int Memory::AccessTime<u8>(const u32 addr);
template int Memory::AccessTime<u16>(const u32 addr);
template int Memory::AccessTime<u32>(const u32 addr);

void Memory::UpdateWaitStates() {
    auto WaitStates = [this](int shift) {
        u16 mask = 0x3 << shift;
        if ((waitcnt.v & mask) == mask) {
            return 8;
        } else {
            return 4 - ((waitcnt.v & mask) >> shift);
        }
    };

    wait_state_sram = 1 + WaitStates(0);
    wait_state_n[0] = 1 + WaitStates(2);
    wait_state_s[0] = 1 + ((waitcnt.v & 0x010) ? 1 : 2);
    wait_state_n[1] = 1 + WaitStates(5);
    wait_state_s[1] = 1 + ((waitcnt.v & 0x080) ? 1 : 4);
    wait_state_n[2] = 1 + WaitStates(8);
    wait_state_s[2] = 1 + ((waitcnt.v & 0x400) ? 1 : 8);
}

template <>
u32 Memory::ReadIO(const u32 addr) const {
    // Unaligned accesses are rotated.
    return RotateRight(ReadIO<u16>(addr & ~0x1) | (ReadIO<u16>((addr & ~0x1) + 2) << 16), (addr & 0x3) * 8);
}

template <>
u8 Memory::ReadIO(const u32 addr) const {
    return ReadIO<u16>(addr & ~0x1) >> (8 * (addr & 0x1));
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
    u16 value;
    switch (addr & ~0x1) {
    case WAITCNT:
        value = waitcnt.Read();
        break;
    case HALTCNT:
        value = haltcnt.Read();
        break;
    default:
        value = 0x0000;
        break;
    }

    // Unaligned accesses are rotated.
    return RotateRight(value, (addr & 0x1) * 8);
}

template <>
void Memory::WriteIO(const u32 addr, const u16 data, const u16 mask) {
    switch (addr & ~0x1) {
    case WAITCNT:
        waitcnt.Write(data, mask);
        UpdateWaitStates();
        break;
    case HALTCNT:
        haltcnt.Write(data, mask);
        break;
    default:
        break;
    }
}

} // End namespace Gba
