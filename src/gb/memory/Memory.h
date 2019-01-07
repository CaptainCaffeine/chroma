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

#pragma once

#include <vector>
#include <array>
#include <algorithm>
#include <memory>

#include "common/CommonTypes.h"
#include "gb/core/Enums.h"

namespace Gb {

class CartridgeHeader;
class GameBoy;
class Rtc;

class Memory {
public:
    Memory(const CartridgeHeader& header, const std::vector<u8>& _rom, const std::string& _save_path,
           GameBoy& _gameboy);
    ~Memory();

    unsigned int double_speed = 0;

    u8 ReadMem(const u16 addr) const;
    void WriteMem(const u16 addr, const u8 data);

    void ToggleCpuSpeed() {
        speed_switch = (speed_switch ^ 0x80) & 0x80;
        double_speed ^= 1;
    };

    // Interrupt functions
    void RequestInterrupt(Interrupt intr) {
        if (!IF_written_this_cycle) { interrupt_flags |= static_cast<unsigned int>(intr); }
    }
    void ClearInterrupt(Interrupt intr) {
        if (!IF_written_this_cycle) { interrupt_flags &= ~static_cast<unsigned int>(intr); }
    }
    bool IsPending(Interrupt intr) const {
        return interrupt_flags & interrupt_enable & static_cast<unsigned int>(intr);
    }
    bool RequestedEnabledInterrupts() const { return interrupt_flags & interrupt_enable; }
    bool IF_written_this_cycle = false;

    // DMA functions
    void UpdateOamDma();
    void UpdateHdma();
    bool HdmaInProgress() const { return hdma_state == DmaState::Active || hdma_state == DmaState::Starting; }
    void SignalHdma();

    // LCD functions
    template<typename DestIter>
    void CopyFromVram(const u16 start_addr, const std::size_t num_bytes, const int bank_num, DestIter dest) const {
        std::copy_n(vram.cbegin() + (start_addr - 0x8000) + 0x2000 * bank_num, num_bytes, dest);
    }

private:
    GameBoy& gameboy;

    const MBC mbc_mode;
    const bool ext_ram_present;
    const bool rtc_present;
    const bool rumble_present;
    const int num_rom_banks;
    const int num_ram_banks;

    const std::vector<u8>& rom;
    std::vector<u8> vram;
    std::vector<u8> wram;
    std::vector<u8> hram;
    std::vector<u8> ext_ram;
    std::unique_ptr<Rtc> rtc;

    const std::string& save_path;

    // Init functions
    void IORegisterInit();
    void VramInit();

    // I/O register functions
    u8 ReadIORegisters(const u16 addr) const;
    void WriteIORegisters(const u16 addr, const u8 data);

    // DMA utilities
    enum class DmaState {Inactive, Starting, Active, Paused};
    enum class Bus {None, External, Vram};
    DmaState oam_dma_state = DmaState::Inactive;
    Bus dma_bus_block = Bus::None;

    u16 oam_transfer_addr;
    u8 oam_transfer_byte;
    unsigned int bytes_read = 160;

    enum class HdmaType {Gdma, Hdma};
    DmaState hdma_state = DmaState::Inactive;
    HdmaType hdma_type;
    bool hdma_reg_written = false;
    int bytes_to_copy = 0, hblank_bytes = 0;

    void InitHdma();
    void ExecuteHdma();
    u8 DmaCopy(const u16 addr) const;

    // MBC/Saving functions
    void ReadSaveFile(unsigned int cart_ram_size);
    void WriteSaveFile();

    u8 ReadExternalRam(const u16 addr) const;
    void WriteExternalRam(const u16 addr, const u8 data);
    void WriteMbcControlRegisters(const u16 addr, const u8 data);

public:
    // IO registers
    static constexpr u16 P1     = 0xFF00;

    static constexpr u16 SB     = 0xFF01;
    static constexpr u16 SC     = 0xFF02;

    static constexpr u16 DIV    = 0xFF04;
    static constexpr u16 TIMA   = 0xFF05;
    static constexpr u16 TMA    = 0xFF06;
    static constexpr u16 TAC    = 0xFF07;

    static constexpr u16 IF     = 0xFF0F;
    u8 interrupt_flags = 0x01;

    static constexpr u16 NR10   = 0xFF10;
    static constexpr u16 NR11   = 0xFF11;
    static constexpr u16 NR12   = 0xFF12;
    static constexpr u16 NR13   = 0xFF13;
    static constexpr u16 NR14   = 0xFF14;

    static constexpr u16 NR21   = 0xFF16;
    static constexpr u16 NR22   = 0xFF17;
    static constexpr u16 NR23   = 0xFF18;
    static constexpr u16 NR24   = 0xFF19;

    static constexpr u16 NR30   = 0xFF1A;
    static constexpr u16 NR31   = 0xFF1B;
    static constexpr u16 NR32   = 0xFF1C;
    static constexpr u16 NR33   = 0xFF1D;
    static constexpr u16 NR34   = 0xFF1E;

    static constexpr u16 NR41   = 0xFF20;
    static constexpr u16 NR42   = 0xFF21;
    static constexpr u16 NR43   = 0xFF22;
    static constexpr u16 NR44   = 0xFF23;

    static constexpr u16 NR50   = 0xFF24;
    static constexpr u16 NR51   = 0xFF25;
    static constexpr u16 NR52   = 0xFF26;

    static constexpr u16 WAVE_0 = 0xFF30;
    static constexpr u16 WAVE_1 = 0xFF31;
    static constexpr u16 WAVE_2 = 0xFF32;
    static constexpr u16 WAVE_3 = 0xFF33;
    static constexpr u16 WAVE_4 = 0xFF34;
    static constexpr u16 WAVE_5 = 0xFF35;
    static constexpr u16 WAVE_6 = 0xFF36;
    static constexpr u16 WAVE_7 = 0xFF37;
    static constexpr u16 WAVE_8 = 0xFF38;
    static constexpr u16 WAVE_9 = 0xFF39;
    static constexpr u16 WAVE_A = 0xFF3A;
    static constexpr u16 WAVE_B = 0xFF3B;
    static constexpr u16 WAVE_C = 0xFF3C;
    static constexpr u16 WAVE_D = 0xFF3D;
    static constexpr u16 WAVE_E = 0xFF3E;
    static constexpr u16 WAVE_F = 0xFF3F;

    static constexpr u16 LCDC   = 0xFF40;
    static constexpr u16 STAT   = 0xFF41;
    static constexpr u16 SCY    = 0xFF42;
    static constexpr u16 SCX    = 0xFF43;
    static constexpr u16 LY     = 0xFF44;
    static constexpr u16 LYC    = 0xFF45;
    static constexpr u16 DMA    = 0xFF46;
    u8 oam_dma_start;
    static constexpr u16 BGP    = 0xFF47;
    static constexpr u16 OBP0   = 0xFF48;
    static constexpr u16 OBP1   = 0xFF49;
    static constexpr u16 WY     = 0xFF4A;
    static constexpr u16 WX     = 0xFF4B;

    static constexpr u16 KEY1   = 0xFF4D;
    u8 speed_switch = 0x00;
    static constexpr u16 VBK    = 0xFF4F;
    unsigned int vram_bank_num = 0x00;

    static constexpr u16 HDMA1  = 0xFF51;
    u8 hdma_source_hi = 0xFF;
    static constexpr u16 HDMA2  = 0xFF52;
    u8 hdma_source_lo = 0xFF;
    static constexpr u16 HDMA3  = 0xFF53;
    u8 hdma_dest_hi = 0xFF;
    static constexpr u16 HDMA4  = 0xFF54;
    u8 hdma_dest_lo = 0xFF;
    static constexpr u16 HDMA5  = 0xFF55;
    u8 hdma_control = 0xFF;

    static constexpr u16 RP     = 0xFF56;
    u8 infrared = 0x02;

    static constexpr u16 BGPI   = 0xFF68;
    static constexpr u16 BGPD   = 0xFF69;
    static constexpr u16 OBPI   = 0xFF6A;
    static constexpr u16 OBPD   = 0xFF6B;

    static constexpr u16 SVBK   = 0xFF70;
    unsigned int wram_bank_num = 0x00;

    u8 interrupt_enable = 0x00;

    // Undocumented CGB registers
    static constexpr u16 UNDOC0 = 0xFF6C;
    static constexpr u16 UNDOC1 = 0xFF72;
    static constexpr u16 UNDOC2 = 0xFF73;
    static constexpr u16 UNDOC3 = 0xFF74;
    static constexpr u16 UNDOC4 = 0xFF75;
    static constexpr u16 UNDOC5 = 0xFF76;
    static constexpr u16 UNDOC6 = 0xFF77;
    std::array<u8, 5> undocumented{{0x00, 0x00, 0x00, 0x00, 0x00}};

    // ******** MBC control registers ******** 
    int rom_bank_num = 0x01;
    int ram_bank_num = 0x00;
    bool ext_ram_enabled = false;

    // MBC1
    int upper_bits = 0x00;
    bool ram_bank_mode = false;
};

} // End namespace Gb
