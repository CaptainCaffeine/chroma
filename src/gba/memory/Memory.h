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

#pragma once

#include <vector>
#include <array>
#include <string>
#include <functional>

#include "common/CommonTypes.h"
#include "common/CommonFuncs.h"
#include "gba/memory/IOReg.h"
#include "gba/core/Enums.h"

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

    enum class SaveType;
    SaveType save_type;
    const std::string& save_path;

    int eeprom_addr_len = 0;
    std::vector<u8> eeprom_bitstream;
    u16 eeprom_ready = 0x1;

    int eeprom_read_pos = 64;
    u64 eeprom_read_buffer = 0x0;

    static constexpr u32 flash_cmd_addr1    = 0x0E00'5555;
    static constexpr u32 flash_cmd_addr2    = 0x0E00'2AAA;
    static constexpr u32 flash_man_addr     = 0x0E00'0000;
    static constexpr u32 flash_dev_addr     = 0x0E00'0001;
    static constexpr u16 panasonic_id       = 0x1B32;
    static constexpr u16 sanyo_id           = 0x1362;
    static constexpr int flash_erase_cycles = 30000; // Slightly less than 2ms.
    static constexpr int flash_write_cycles = 300; // Around 18us.

    enum class FlashState {NotStarted, Starting, Ready, Command};
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

    FlashState flash_state = FlashState::NotStarted;
    FlashCmd last_flash_cmd = FlashCmd::None;
    u32 sram_addr_mask;
    bool flash_id_mode = false;
    u16 chip_id = panasonic_id;
    int bank_num = 0;

    struct DelayedOp {
        DelayedOp(int _cycles, std::function<void()> _action)
                : cycles(_cycles)
                , action(std::move(_action)) {}

        int cycles;
        std::function<void()> action;
    };
    DelayedOp delayed_op{0, [](){}};

    static constexpr unsigned int kbyte = 1024;
    static constexpr unsigned int mbyte = kbyte * kbyte;

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

    enum class SaveType {Unknown,
                         SRam,
                         Eeprom,
                         Flash,
                         Flash128,
                         None};

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

    // IO registers
    static constexpr u32 DISPCNT     = 0x0400'0000;
    static constexpr u32 GREENSWAP   = 0x0400'0002;
    static constexpr u32 DISPSTAT    = 0x0400'0004;
    static constexpr u32 VCOUNT      = 0x0400'0006;

    static constexpr u32 BG0CNT      = 0x0400'0008;
    static constexpr u32 BG1CNT      = 0x0400'000A;
    static constexpr u32 BG2CNT      = 0x0400'000C;
    static constexpr u32 BG3CNT      = 0x0400'000E;
    static constexpr u32 BG0HOFS     = 0x0400'0010;
    static constexpr u32 BG0VOFS     = 0x0400'0012;
    static constexpr u32 BG1HOFS     = 0x0400'0014;
    static constexpr u32 BG1VOFS     = 0x0400'0016;
    static constexpr u32 BG2HOFS     = 0x0400'0018;
    static constexpr u32 BG2VOFS     = 0x0400'001A;
    static constexpr u32 BG3HOFS     = 0x0400'001C;
    static constexpr u32 BG3VOFS     = 0x0400'001E;

    static constexpr u32 BG2PA       = 0x0400'0020;
    static constexpr u32 BG2PB       = 0x0400'0022;
    static constexpr u32 BG2PC       = 0x0400'0024;
    static constexpr u32 BG2PD       = 0x0400'0026;
    static constexpr u32 BG2X_L      = 0x0400'0028;
    static constexpr u32 BG2X_H      = 0x0400'002A;
    static constexpr u32 BG2Y_L      = 0x0400'002C;
    static constexpr u32 BG2Y_H      = 0x0400'002E;

    static constexpr u32 BG3PA       = 0x0400'0030;
    static constexpr u32 BG3PB       = 0x0400'0032;
    static constexpr u32 BG3PC       = 0x0400'0034;
    static constexpr u32 BG3PD       = 0x0400'0036;
    static constexpr u32 BG3X_L      = 0x0400'0038;
    static constexpr u32 BG3X_H      = 0x0400'003A;
    static constexpr u32 BG3Y_L      = 0x0400'003C;
    static constexpr u32 BG3Y_H      = 0x0400'003E;

    static constexpr u32 WIN0H       = 0x0400'0040;
    static constexpr u32 WIN1H       = 0x0400'0042;
    static constexpr u32 WIN0V       = 0x0400'0044;
    static constexpr u32 WIN1V       = 0x0400'0046;
    static constexpr u32 WININ       = 0x0400'0048;
    static constexpr u32 WINOUT      = 0x0400'004A;

    static constexpr u32 MOSAIC      = 0x0400'004C;
    static constexpr u32 BLDCNT      = 0x0400'0050;
    static constexpr u32 BLDALPHA    = 0x0400'0052;
    static constexpr u32 BLDY        = 0x0400'0054;

    static constexpr u32 SOUND1CNT_L = 0x0400'0060;
    static constexpr u32 SOUND1CNT_H = 0x0400'0062;
    static constexpr u32 SOUND1CNT_X = 0x0400'0064;
    static constexpr u32 SOUND2CNT_L = 0x0400'0068;
    static constexpr u32 SOUND2CNT_H = 0x0400'006C;
    static constexpr u32 SOUND3CNT_L = 0x0400'0070;
    static constexpr u32 SOUND3CNT_H = 0x0400'0072;
    static constexpr u32 SOUND3CNT_X = 0x0400'0074;
    static constexpr u32 SOUND4CNT_L = 0x0400'0078;
    static constexpr u32 SOUND4CNT_H = 0x0400'007C;

    static constexpr u32 SOUNDCNT_L  = 0x0400'0080;
    static constexpr u32 SOUNDCNT_H  = 0x0400'0082;
    static constexpr u32 SOUNDCNT_X  = 0x0400'0084;
    static constexpr u32 SOUNDBIAS   = 0x0400'0088;

    static constexpr u32 WAVE_RAM0_L = 0x0400'0090;
    static constexpr u32 WAVE_RAM0_H = 0x0400'0092;
    static constexpr u32 WAVE_RAM1_L = 0x0400'0094;
    static constexpr u32 WAVE_RAM1_H = 0x0400'0096;
    static constexpr u32 WAVE_RAM2_L = 0x0400'0098;
    static constexpr u32 WAVE_RAM2_H = 0x0400'009A;
    static constexpr u32 WAVE_RAM3_L = 0x0400'009C;
    static constexpr u32 WAVE_RAM3_H = 0x0400'009E;

    static constexpr u32 FIFO_A_L    = 0x0400'00A0;
    static constexpr u32 FIFO_A_H    = 0x0400'00A2;
    static constexpr u32 FIFO_B_L    = 0x0400'00A4;
    static constexpr u32 FIFO_B_H    = 0x0400'00A6;

    static constexpr u32 DMA0SAD_L   = 0x0400'00B0;
    static constexpr u32 DMA0SAD_H   = 0x0400'00B2;
    static constexpr u32 DMA0DAD_L   = 0x0400'00B4;
    static constexpr u32 DMA0DAD_H   = 0x0400'00B6;
    static constexpr u32 DMA0CNT_L   = 0x0400'00B8;
    static constexpr u32 DMA0CNT_H   = 0x0400'00BA;

    static constexpr u32 DMA1SAD_L   = 0x0400'00BC;
    static constexpr u32 DMA1SAD_H   = 0x0400'00BE;
    static constexpr u32 DMA1DAD_L   = 0x0400'00C0;
    static constexpr u32 DMA1DAD_H   = 0x0400'00C2;
    static constexpr u32 DMA1CNT_L   = 0x0400'00C4;
    static constexpr u32 DMA1CNT_H   = 0x0400'00C6;

    static constexpr u32 DMA2SAD_L   = 0x0400'00C8;
    static constexpr u32 DMA2SAD_H   = 0x0400'00CA;
    static constexpr u32 DMA2DAD_L   = 0x0400'00CC;
    static constexpr u32 DMA2DAD_H   = 0x0400'00CE;
    static constexpr u32 DMA2CNT_L   = 0x0400'00D0;
    static constexpr u32 DMA2CNT_H   = 0x0400'00D2;

    static constexpr u32 DMA3SAD_L   = 0x0400'00D4;
    static constexpr u32 DMA3SAD_H   = 0x0400'00D6;
    static constexpr u32 DMA3DAD_L   = 0x0400'00D8;
    static constexpr u32 DMA3DAD_H   = 0x0400'00DA;
    static constexpr u32 DMA3CNT_L   = 0x0400'00DC;
    static constexpr u32 DMA3CNT_H   = 0x0400'00DE;

    static constexpr u32 TM0CNT_L    = 0x0400'0100;
    static constexpr u32 TM0CNT_H    = 0x0400'0102;
    static constexpr u32 TM1CNT_L    = 0x0400'0104;
    static constexpr u32 TM1CNT_H    = 0x0400'0106;
    static constexpr u32 TM2CNT_L    = 0x0400'0108;
    static constexpr u32 TM2CNT_H    = 0x0400'010A;
    static constexpr u32 TM3CNT_L    = 0x0400'010C;
    static constexpr u32 TM3CNT_H    = 0x0400'010E;

    static constexpr u32 SIOMULTI0   = 0x0400'0120; // Also SIODATA32_L
    static constexpr u32 SIOMULTI1   = 0x0400'0122; // Also SIODATA32_H
    static constexpr u32 SIOMULTI2   = 0x0400'0124;
    static constexpr u32 SIOMULTI3   = 0x0400'0126;
    static constexpr u32 SIOCNT      = 0x0400'0128;
    static constexpr u32 SIOMLTSEND  = 0x0400'012A; // Also SIODATA8

    static constexpr u32 KEYINPUT    = 0x0400'0130;
    static constexpr u32 KEYCNT      = 0x0400'0132;

    static constexpr u32 RCNT        = 0x0400'0134;
    static constexpr u32 JOYCNT      = 0x0400'0140;
    static constexpr u32 JOYRECV_L   = 0x0400'0150;
    static constexpr u32 JOYRECV_H   = 0x0400'0152;
    static constexpr u32 JOYTRANS_L  = 0x0400'0154;
    static constexpr u32 JOYTRANS_H  = 0x0400'0156;
    static constexpr u32 JOYSTAT     = 0x0400'0158;

    static constexpr u32 IE          = 0x0400'0200;
    IOReg intr_enable = {0x0000, 0x3FFF, 0x3FFF};

    static constexpr u32 IF          = 0x0400'0202;
    IOReg intr_flags = {0x0000, 0x3FFF, 0x3FFF};

    static constexpr u32 WAITCNT     = 0x0400'0204;
    IOReg waitcnt = {0x0000, 0x5FFF, 0x5FFF};

    static constexpr u32 IME         = 0x0400'0208;
    IOReg master_enable = {0x0000, 0x0001, 0x0001};

    static constexpr u32 HALTCNT     = 0x0400'0300; // Also POSTFLG
    IOReg haltcnt = {0x0000, 0x0001, 0x8001};
};

} // End namespace Gba
