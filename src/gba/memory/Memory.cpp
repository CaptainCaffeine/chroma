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
        , io_regs(io_size / sizeof(u32))
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

} // End namespace Gba
