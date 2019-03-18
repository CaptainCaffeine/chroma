// This file is a part of Chroma.
// Copyright (C) 2017-2019 Matthew Murray
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
#include <string>
#include <functional>

#include "common/CommonTypes.h"
#include "common/CommonFuncs.h"
#include "gba/memory/IOReg.h"
#include "gba/memory/MemDefs.h"

namespace Gba {

class Core;

class Memory {
public:
    Memory(const std::vector<u32>& _bios, const std::vector<u16>& _rom, const std::string& _save_path, Core& _core);
    ~Memory();

    u32 transfer_reg = 0x0;

    template <typename T>
    T ReadMem(const u32 addr, bool dma = false);
    template <typename T>
    void WriteMem(const u32 addr, const T data, bool dma = false);
    template <typename T>
    int AccessTime(const u32 addr, AccessType access_type = AccessType::Normal, bool force_sequential = false);

    void MakeNextAccessSequential(u32 addr) { last_addr = addr; }
    void MakeNextAccessNonsequential() { last_addr = 0; }
    bool LastAccessWasInRom() const { return last_addr >= BaseAddr::Rom; }

    bool PrefetchEnabled() const { return waitcnt & 0x4000; }
    void RunPrefetch(int cycles);
    void FlushPrefetchBuffer() { prefetch_cycles = 0; prefetched_opcodes = 0; last_addr = 0; }

    bool InterruptMasterEnable() const { return master_enable.v; }
    bool PendingInterrupts() const { return intr_flags & intr_enable; }
    void RequestInterrupt(u16 intr) { intr_flags |= intr; };
    bool InterruptEnabled(u16 intr) const { return intr_enable & intr; };

    bool EepromAddr(u32 addr) const { return rom_size <= 16 * mbyte || addr >= 0x0DFF'FF00; }
    void ParseEepromCommand();

    void DelayedSaveOp(int cycles);

    const std::vector<u16>& PramReference() const { return pram; }
    const std::vector<u16>& VramReference() const { return vram; }
    const std::vector<u32>& OamReference() const { return oam; }

    static bool CheckNintendoLogo(const std::vector<u8>& rom_header) noexcept;
    static void CheckHeader(const std::vector<u16>& rom_header);

private:
    Core& core;

    const std::vector<u32>& bios;
    std::vector<u16> xram;
    std::vector<u32> iram;
    std::vector<u16> pram;
    std::vector<u16> vram;
    std::vector<u32> oam;
    const std::vector<u16>& rom;
    std::vector<u8> sram;
    std::vector<u64> eeprom;

    u32 last_addr = 0x0;
    int prefetch_cycles = 0;
    int prefetched_opcodes = 0;

    std::array<int, 3> wait_state_n;
    std::array<int, 3> wait_state_s;
    int wait_state_sram;

    const unsigned int rom_size;
    u32 rom_addr_mask;

    bool gpio_present = false;

    IOReg intr_enable = {0x0000, 0x3FFF, 0x3FFF};
    IOReg intr_flags = {0x0000, 0x3FFF, 0x3FFF};
    IOReg waitcnt = {0x0000, 0x5FFF, 0x5FFF};
    IOReg master_enable = {0x0000, 0x0001, 0x0001};
    IOReg haltcnt = {0x0000, 0x0001, 0x8001};

    IOReg gpio_data = {0x0000, 0x000F, 0x000F};
    IOReg gpio_direction = {0x0000, 0x000F, 0x000F};
    IOReg gpio_readable = {0x0000, 0x0001, 0x0001};

    enum GpioAddr : u32 {Data      = 0x0800'00C4,
                         Direction = 0x0800'00C6,
                         Control   = 0x0800'00C8};

    enum class SaveType {Unknown,
                         SRam,
                         Eeprom,
                         Flash,
                         Flash128,
                         None};

    enum class FlashState {NotStarted,
                           Starting,
                           Ready,
                           Command};

    enum FlashAddr : u32 {Command1     = 0x0E00'5555,
                          Command2     = 0x0E00'2AAA,
                          Manufacturer = 0x0E00'0000,
                          Device       = 0x0E00'0001};

    enum FlashId : u16 {Panasonic = 0x1B32,
                        Sanyo     = 0x1362};

    enum FlashCmd {Start1      = 0xAA,
                   Start2      = 0x55,
                   EnterIdMode = 0x90,
                   ExitIdmode  = 0xF0,
                   Erase       = 0x80,
                   EraseChip   = 0x10,
                   EraseSector = 0x30,
                   Write       = 0xA0,
                   BankSwitch  = 0xB0,
                   None        = 0x00};

    static constexpr int eeprom_write_cycles = 108368; // 6.46ms
    static constexpr int flash_erase_cycles  = 30000; // 1.79ms
    static constexpr int flash_write_cycles  = 300; // 17.9us

    SaveType save_type;
    const std::string& save_path;

    int eeprom_addr_len = 0;
    std::vector<u8> eeprom_bitstream;
    u16 eeprom_ready = 0x1;
    int eeprom_read_pos = 64;
    u64 eeprom_read_buffer = 0x0;

    FlashState flash_state = FlashState::NotStarted;
    FlashCmd last_flash_cmd = FlashCmd::None;
    u32 sram_addr_mask;
    bool flash_id_mode = false;
    FlashId chip_id = FlashId::Panasonic;
    int bank_num = 0;

    struct DelayedOp {
        DelayedOp(int _cycles, std::function<void()> _action)
                : cycles(_cycles)
                , action(std::move(_action)) {}

        int cycles;
        std::function<void()> action;
    };
    DelayedOp delayed_op{0, [](){}};

    enum class Region {Bios   = 0x0,
                       XRam   = 0x2,
                       IRam   = 0x3,
                       IO     = 0x4,
                       PRam   = 0x5,
                       VRam   = 0x6,
                       Oam    = 0x7,
                       Rom0_l = 0x8,
                       Rom0_h = 0x9,
                       Rom1_l = 0xA,
                       Rom1_h = 0xB,
                       Rom2_l = 0xC,
                       Eeprom = 0xD,
                       SRam_l = 0xE,
                       SRam_h = 0xF};

    enum RegionSize {bios_size    = 16 * kbyte,
                     xram_size    = 256 * kbyte,
                     iram_size    = 32 * kbyte,
                     io_size      = kbyte,
                     pram_size    = kbyte,
                     vram_size    = 96 * kbyte,
                     oam_size     = kbyte,
                     rom_max_size = 32 * mbyte,
                     sram_size    = 32 * kbyte,
                     flash_size   = 64 * kbyte};

    enum AddressMask : u32 {bios_addr_mask  = bios_size - 1,
                            xram_addr_mask  = xram_size - 1,
                            iram_addr_mask  = iram_size - 1,
                            io_addr_mask    = io_size - 1,
                            pram_addr_mask  = pram_size - 1,
                            vram_addr_mask1 = 0x0000'FFFF,
                            vram_addr_mask2 = 0x0001'7FFF,
                            oam_addr_mask   = oam_size - 1};

    static constexpr Region GetRegion(const u32 addr) {
        constexpr u32 region_offset = 24;
        return static_cast<Region>(addr >> region_offset);
    }

    template <typename AccessWidth, typename BusWidth>
    AccessWidth ReadRegion(const std::vector<BusWidth>& region, const u32 region_mask, const u32 addr) const;
    template <typename AccessWidth, typename BusWidth>
    void WriteRegion(std::vector<BusWidth>& region, const u32 region_mask, const u32 addr, const AccessWidth data);

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
    T ReadRom(const u32 addr) const {
        if ((addr & rom_addr_mask) < rom_size) {
            return ReadRegion<T>(rom, rom_addr_mask, addr);
        } else {
            return 0;
        }
    }
    template <typename T>
    T ReadSRam(const u32 addr) const { return sram[bank_num * flash_size + (addr & sram_addr_mask)] * 0x0101'0101; }

    template <typename T>
    void WriteXRam(const u32 addr, const T data) { WriteRegion(xram, xram_addr_mask, addr, data); }
    template <typename T>
    void WriteIRam(const u32 addr, const T data) { WriteRegion(iram, iram_addr_mask, addr, data); }
    template <typename T>
    void WriteIO(const u32 addr, const T data, const u16 mask = 0xFFFF);
    template <typename T>
    void WritePRam(const u32 addr, const T data) { WriteRegion(pram, pram_addr_mask, addr, data); }
    template <typename T>
    void WriteVRam(const u32 addr, const T data);
    template <typename T>
    void WriteOam(const u32 addr, const T data);
    template <typename T>
    void WriteSRam(const u32 addr, const T data) {
        sram[bank_num * flash_size + (addr & sram_addr_mask)] = RotateRight(data, (addr & (sizeof(T) - 1)) * 8);
    }
    template <typename T>
    void WriteFlash(const u32 addr, const T data);

    void UpdateWaitStates();
    u32 ReadOpenBus() const;

    void ReadSaveFile();
    void WriteSaveFile() const;
    void CheckSaveOverrides();
    void CheckHardwareOverrides();
    void InitSRam();
    void InitFlash();
    u16 ParseEepromAddr(int stream_size, bool read_request);
    void InitEeprom(int stream_size, int non_addr_bits);

    void WriteGpio(const u32 addr, const u16 data);
    void UpdateGpioDirections();
    void UpdateGpioReadable();
    static bool InGpioAddrRange(u32 addr) { return (addr & 0xFFFF'FFF1) == 0x0800'00C0; }
};

} // End namespace Gba
