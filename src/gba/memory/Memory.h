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

#pragma once

#include <vector>

#include "common/CommonTypes.h"

namespace Gba {

class Memory {
public:
    Memory(const std::vector<u16>& rom_contents);

    template <typename T>
    T ReadMem(const u32 addr) const;
    template <typename T>
    void WriteMem(const u32 addr, const T data);

    static bool CheckNintendoLogo(const std::vector<u8>& rom_header) noexcept;
    static void CheckHeader(const std::vector<u16>& rom_header);
private:
    const std::vector<u16>& rom;
    std::vector<u16> xram;
    std::vector<u32> iram;
    std::vector<u32> io_regs;
    std::vector<u16> pram;
    std::vector<u16> vram;
    std::vector<u32> oam;

    static constexpr unsigned int kbyte = 1024;
    static constexpr unsigned int mbyte = kbyte * kbyte;

    enum class Region {Bios = 0x0,
                       XRam = 0x2,
                       IRam = 0x3,
                       IO   = 0x4,
                       PRam = 0x5,
                       VRam = 0x6,
                       Oam  = 0x7,
                       Rom0 = 0x8,
                       Rom1 = 0xA,
                       Rom2 = 0xC,
                       SRam = 0xE};

    enum RegionSize {rom_size  = 32 * mbyte,
                     xram_size = 256 * kbyte,
                     iram_size = 32 * kbyte,
                     io_size   = kbyte,
                     pram_size = kbyte,
                     vram_size = 96 * kbyte,
                     oam_size  = kbyte};

    enum AddressMask : u32 {rom_addr_mask   = rom_size - 1,
                            xram_addr_mask  = xram_size - 1,
                            iram_addr_mask  = iram_size - 1,
                            io_addr_mask    = io_size - 1,
                            pram_addr_mask  = pram_size - 1,
                            vram_addr_mask1 = 0x0000FFFF,
                            vram_addr_mask2 = 0x00017FFF,
                            oam_addr_mask   = oam_size - 1};

    static constexpr u32 region_offset = 24;
    static constexpr Region GetRegion(const u32 addr) {
        return static_cast<Region>((addr >> region_offset) & 0x0F);
    }

    template <typename AccessWidth, typename BusWidth>
    AccessWidth ReadRegion(const std::vector<BusWidth>& region, const AddressMask region_mask, const u32 addr) const;
    template <typename AccessWidth, typename BusWidth>
    void WriteRegion(std::vector<BusWidth>& region, const AddressMask region_mask, const u32 addr, const AccessWidth data);

    template <typename T>
    T ReadRom(const u32 addr) const { return ReadRegion<T>(rom, rom_addr_mask, addr); }
    template <typename T>
    T ReadXRam(const u32 addr) const { return ReadRegion<T>(xram, xram_addr_mask, addr); }
    template <typename T>
    T ReadIRam(const u32 addr) const { return ReadRegion<T>(iram, iram_addr_mask, addr); }
    template <typename T>
    T ReadIO(const u32 addr) const { return ReadRegion<T>(io_regs, io_addr_mask, addr); }
    template <typename T>
    T ReadPRam(const u32 addr) const { return ReadRegion<T>(pram, pram_addr_mask, addr); }
    template <typename T>
    T ReadVRam(const u32 addr) const {
        return ReadRegion<T>(vram, (addr & 0x00010000) ? vram_addr_mask2 : vram_addr_mask1, addr);
    }
    template <typename T>
    T ReadOam(const u32 addr) const { return ReadRegion<T>(oam, oam_addr_mask, addr); }

    template <typename T>
    void WriteXRam(const u32 addr, const T data) { WriteRegion(xram, xram_addr_mask, addr, data); }
    template <typename T>
    void WriteIRam(const u32 addr, const T data) { WriteRegion(iram, iram_addr_mask, addr, data); }
    template <typename T>
    void WriteIO(const u32 addr, const T data) { WriteRegion(io_regs, io_addr_mask, addr, data); }
    template <typename T>
    void WritePRam(const u32 addr, const T data) { WriteRegion(pram, pram_addr_mask, addr, data); }
    template <typename T>
    void WriteVRam(const u32 addr, const T data) {
        WriteRegion(vram, (addr & 0x00010000) ? vram_addr_mask2 : vram_addr_mask1, addr, data);
    }
    template <typename T>
    void WriteOam(const u32 addr, const T data) { WriteRegion(oam, oam_addr_mask, addr, data); }
};

} // End namespace Gba
