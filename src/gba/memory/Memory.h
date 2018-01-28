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
#include <array>

#include "common/CommonTypes.h"
#include "gba/memory/IOReg.h"

namespace Gba {

class Core;

class Memory {
public:
    Memory(const std::vector<u32>& _bios, const std::vector<u16>& _rom, Core& _core);

    template <typename T>
    T ReadMem(const u32 addr) const;
    template <typename T>
    void WriteMem(const u32 addr, const T data);
    template <typename T>
    int AccessTime(const u32 addr);

    void MakeNextAccessSequential(u32 addr) { last_addr = addr; }

    bool InterruptMasterEnable() const { return master_enable.v; }
    bool PendingInterrupts() const { return intr_flags.v & intr_enable.v; }
    void RequestInterrupt(u16 intr) { intr_flags.v |= intr; };

    static bool CheckNintendoLogo(const std::vector<u8>& rom_header) noexcept;
    static void CheckHeader(const std::vector<u16>& rom_header);
private:
    const std::vector<u32>& bios;
    std::vector<u16> xram;
    std::vector<u32> iram;
    std::vector<u16> pram;
    std::vector<u16> vram;
    std::vector<u32> oam;
    const std::vector<u16>& rom;

    Core& core;

    u32 last_addr = 0x0;

    std::array<int, 3> wait_state_n;
    std::array<int, 3> wait_state_s;
    int wait_state_sram;

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

    enum RegionSize {bios_size = 16 * kbyte,
                     xram_size = 256 * kbyte,
                     iram_size = 32 * kbyte,
                     io_size   = kbyte,
                     pram_size = kbyte,
                     vram_size = 96 * kbyte,
                     oam_size  = kbyte,
                     rom_size  = 32 * mbyte};

    enum AddressMask : u32 {bios_addr_mask  = bios_size - 1,
                            xram_addr_mask  = xram_size - 1,
                            iram_addr_mask  = iram_size - 1,
                            io_addr_mask    = io_size - 1,
                            pram_addr_mask  = pram_size - 1,
                            vram_addr_mask1 = 0x0000'FFFF,
                            vram_addr_mask2 = 0x0001'7FFF,
                            oam_addr_mask   = oam_size - 1,
                            rom_addr_mask   = rom_size - 1};

    static constexpr u32 io_max = 0x0400'0000 + io_size;

    static constexpr u32 region_offset = 24;
    static constexpr Region GetRegion(const u32 addr) {
        return static_cast<Region>((addr >> region_offset) & 0x0F);
    }

    template <typename AccessWidth, typename BusWidth>
    AccessWidth ReadRegion(const std::vector<BusWidth>& region, const AddressMask region_mask, const u32 addr) const;
    template <typename AccessWidth, typename BusWidth>
    void WriteRegion(std::vector<BusWidth>& region, const AddressMask region_mask, const u32 addr, const AccessWidth data);

    template <typename T>
    T ReadBios(const u32 addr) const;
    template <typename T>
    T ReadXRam(const u32 addr) const { return ReadRegion<T>(xram, xram_addr_mask, addr); }
    template <typename T>
    T ReadIRam(const u32 addr) const { return ReadRegion<T>(iram, iram_addr_mask, addr); }
    template <typename T>
    T ReadIO(const u32 addr) const;
    template <typename T>
    T ReadPRam(const u32 addr) const { return ReadRegion<T>(pram, pram_addr_mask, addr); }
    template <typename T>
    T ReadVRam(const u32 addr) const {
        return ReadRegion<T>(vram, (addr & 0x0001'0000) ? vram_addr_mask2 : vram_addr_mask1, addr);
    }
    template <typename T>
    T ReadOam(const u32 addr) const { return ReadRegion<T>(oam, oam_addr_mask, addr); }
    template <typename T>
    T ReadRom(const u32 addr) const { return ReadRegion<T>(rom, rom_addr_mask, addr); }

    template <typename T>
    void WriteXRam(const u32 addr, const T data) { WriteRegion(xram, xram_addr_mask, addr, data); }
    template <typename T>
    void WriteIRam(const u32 addr, const T data) { WriteRegion(iram, iram_addr_mask, addr, data); }
    template <typename T>
    void WriteIO(const u32 addr, const T data, const u16 mask = 0xFFFF);
    template <typename T>
    void WritePRam(const u32 addr, const T data) { WriteRegion(pram, pram_addr_mask, addr, data); }
    template <typename T>
    void WriteVRam(const u32 addr, const T data) {
        WriteRegion(vram, (addr & 0x0001'0000) ? vram_addr_mask2 : vram_addr_mask1, addr, data);
    }
    template <typename T>
    void WriteOam(const u32 addr, const T data) { WriteRegion(oam, oam_addr_mask, addr, data); }

    void UpdateWaitStates();

    // IO registers
    static constexpr u32 TM0CNT_L = 0x0400'0100;
    static constexpr u32 TM0CNT_H = 0x0400'0102;
    static constexpr u32 TM1CNT_L = 0x0400'0104;
    static constexpr u32 TM1CNT_H = 0x0400'0106;
    static constexpr u32 TM2CNT_L = 0x0400'0108;
    static constexpr u32 TM2CNT_H = 0x0400'010A;
    static constexpr u32 TM3CNT_L = 0x0400'010C;
    static constexpr u32 TM3CNT_H = 0x0400'010E;

    static constexpr u32 IE = 0x0400'0200;
    IOReg intr_enable = {0x0000, 0x3FFF, 0x3FFF};

    static constexpr u32 IF = 0x0400'0202;
    IOReg intr_flags = {0x0000, 0x3FFF, 0x3FFF};

    static constexpr u32 WAITCNT = 0x0400'0204;
    IOReg waitcnt = {0x0000, 0x5FFF, 0x5FFF};

    static constexpr u32 IME = 0x0400'0208;
    IOReg master_enable = {0x0000, 0x0001, 0x0001};

    // Also POSTFLG.
    static constexpr u32 HALTCNT = 0x0400'0300;
    IOReg haltcnt = {0x0000, 0x0001, 0x8001};
};

} // End namespace Gba
