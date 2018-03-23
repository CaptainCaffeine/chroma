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

#include <array>
#include <stdexcept>
#include <fmt/format.h>

#include "common/CommonFuncs.h"
#include "gb/memory/CartridgeHeader.h"

namespace Gb {

CartridgeHeader::CartridgeHeader(Console& console, const std::vector<u8>& rom, bool multicart_requested) {
    // Determine if this game enables CGB functions. A value of 0xC0 implies the game is CGB-only, and
    // 0x80 implies it can also run on pre-CGB devices. They both have the same effect, as it's up to
    // the game to test if it is running on a pre-CGB device.
    bool cgb_flag = rom[0x0143] == 0xC0 || rom[0x0143] == 0x80;

    // If no console was specified, we emulate a CGB if the game has CGB features, and a DMG otherwise.
    if (console == Console::Default) {
        if (cgb_flag) {
            console = Console::CGB;
        } else {
            console = Console::DMG;
        }
    }

    if (console != Console::DMG && cgb_flag) {
        game_mode = GameMode::CGB;
    } else {
        game_mode = GameMode::DMG;
    }

    // The ROM size is at 0x0148 in cartridge header. Each ROM bank is 16KB.
    num_rom_banks = (0x8000 << rom[0x0148]) / 0x4000;
    if (rom.size() != num_rom_banks * 0x4000) {
        fmt::print("WARNING: Size of provided ROM does not match size given in cartridge header.\n");
    }

    GetRAMSize(rom);
    GetMBCType(rom);
    if (console == Console::DMG && !CheckNintendoLogo(console, rom)) {
        fmt::print("WARNING: Nintendo logo does not match. This ROM would not run on a DMG!\n");
    }
    HeaderChecksum(rom);

    // If the user gave the multicart option and this game reports itself as using an MBC1, emulate an MBC1M instead.
    if (mbc_mode == MBC::MBC1 && multicart_requested) {
        mbc_mode = MBC::MBC1M;
    }

    // MBC2 carts always have 0x00 in the RAM size field.
    if (mbc_mode == MBC::MBC2 && ext_ram_present) {
        ram_size = 0x200;
    }
}

void CartridgeHeader::GetRAMSize(const std::vector<u8>& rom) {
    // The RAM size identifier is at 0x0149 in cartridge header.
    switch (rom[0x0149]) {
    case 0x00:
        // Either no external RAM, or MBC2
        ram_size = 0x00;
        break;
    case 0x01:
        // 2KB external RAM
        ram_size = 0x800;
        break;
    case 0x02:
        // 8KB external RAM
        ram_size = 0x2000;
        break;
    case 0x03:
        // 32KB external RAM - 4 banks
        ram_size = 0x8000;
        break;
    case 0x04:
        // 128KB external RAM - 16 banks
        ram_size = 0x20000;
        break;
    case 0x05:
        // 64KB external RAM - 8 banks
        ram_size = 0x10000;
        break;
    default:
        // I don't know if this happens in official games, but it could happen in homebrew.
        throw std::runtime_error("Unrecognized external RAM quantity given in cartridge header.");
        break;
    }
}

void CartridgeHeader::GetMBCType(const std::vector<u8>& rom) {
    // The MBC type is at 0x0147. The MBC identifier also tells us if this cartridge contains external RAM.
    switch (rom[0x0147]) {
    case 0x00:
        // ROM only, no MBC
        mbc_mode = MBC::None;
        ext_ram_present = false;
        break;
    case 0x01:
        // MBC1, no RAM
        mbc_mode = MBC::MBC1;
        ext_ram_present = false;
        break;
    case 0x02:
    case 0x03:
        // MBC1 with external RAM, 0x03 implies the cart has a battery as well.
        mbc_mode = MBC::MBC1;
        ext_ram_present = true;
        break;
    case 0x05:
        // MBC2, no RAM
        mbc_mode = MBC::MBC2;
        ext_ram_present = false;
        break;
    case 0x06:
        // MBC2 with embedded nybble RAM
        mbc_mode = MBC::MBC2;
        ext_ram_present = true;
        break;
    case 0x08:
    case 0x09:
        // ROM + external RAM, no MBC, 0x09 implies battery as well.
        // This is listed in a few cartridge header tables, but Gekkio claims no official games with this
        // configuration exist. (http://gekkio.fi/blog/2015-02-28-mooneye-gb-cartridge-analysis-tetris.html)
        mbc_mode = MBC::None;
        ext_ram_present = true;
        break;
    case 0x0B:
        // MMM01, no RAM.
        // I can't find any information on this MBC, but it's supposedly present in "Momotarou Collection 2".
        throw std::runtime_error("MMM01 unimplemented.");
        break;
    case 0x0C:
    case 0x0D:
        // MMM01 with external RAM, 0x0D implies battery as well.
        // I can't find any information on this MBC, but it's supposedly present in "Momotarou Collection 2".
        throw std::runtime_error("MMM01 unimplemented.");
        break;
    case 0x0F:
        // MBC3 with timer and battery, no RAM.
        mbc_mode = MBC::MBC3;
        ext_ram_present = false;
        rtc_present = true;
        break;
    case 0x10:
        // MBC3 with RAM, timer, and battery.
        mbc_mode = MBC::MBC3;
        ext_ram_present = true;
        rtc_present = true;
        break;
    case 0x11:
        // MBC3, no RAM.
        mbc_mode = MBC::MBC3;
        ext_ram_present = false;
        break;
    case 0x12:
    case 0x13:
        // MBC3 with external RAM. 0x13 implies battery.
        mbc_mode = MBC::MBC3;
        ext_ram_present = true;
        break;
    case 0x19:
        // MBC5, no RAM.
        mbc_mode = MBC::MBC5;
        ext_ram_present = false;
        break;
    case 0x1C:
        // MBC5 with rumble, no RAM.
        mbc_mode = MBC::MBC5;
        ext_ram_present = false;
        rumble_present = true;
        break;
    case 0x1A:
    case 0x1B:
        // MBC5 with external RAM. 0x1B implies battery.
        mbc_mode = MBC::MBC5;
        ext_ram_present = true;
        break;
    case 0x1D:
    case 0x1E:
        // MBC5 with external RAM and rumble. 0x1E implies battery.
        mbc_mode = MBC::MBC5;
        ext_ram_present = true;
        rumble_present = true;
        break;
    case 0x20:
        // MBC6 with external RAM and battery.
        throw std::runtime_error("MBC6 unimplemented.");
        break;
    case 0x22:
        // MBC7 with external RAM, battery, and accelerometer. Only used by Kirby Tilt n Tumble.
        throw std::runtime_error("MBC7 unimplemented.");
        break;
    case 0xFC:
        // Pocket Camera
        throw std::runtime_error("Pocket Camera unimplemented.");
        break;
    case 0xFD:
        // Bandai TAMA5, used in Tamagotchi games.
        throw std::runtime_error("TAMA5 unimplemented.");
        break;
    case 0xFE:
        // HuC3 with infrared port
        throw std::runtime_error("HuC3 unimplemented.");
        break;
    case 0xFF:
        // HuC1 with external RAM, battery, and infrared port
        throw std::runtime_error("HuC1 unimplemented.");
        break;

    default:
        throw std::runtime_error("Unrecognized MBC.");
    }
}

void CartridgeHeader::HeaderChecksum(const std::vector<u8>& rom) const {
    u8 checksum = 0;
    for (std::size_t i = 0x0134; i < 0x014D; ++i) {
        checksum -= rom[i] + 1;
    }

    // The header checksum at 0x014D must match the value calculated above. This is checked in the boot ROM, and if
    // it does not match the Game Boy locks up.
    if (checksum != rom[0x014D]) {
        fmt::print("WARNING: Header checksum does not match. This ROM would not run on a Game Boy!\n");
    }
}

bool CartridgeHeader::CheckNintendoLogo(const Console console, const std::vector<u8>& rom) noexcept {
    // Calculate the FNV-1a hash of the first or second half of the region in the ROM header where the Nintendo logo
    // is supposed to be (0x0104-0x0133) and compare it to a precalculated hash of the expected logo.
    static constexpr u32 logo_first_half_hash = 0x14BDDD1B;
    static constexpr u32 logo_second_half_hash = 0x9FD20031;
    static constexpr u16 logo_offset = 0x0104;

    if (console == Console::CGB) {
        // The CGB boot ROM only checks the first half (24 bytes) of the logo.
        u32 header_first_half_hash = Fnv1aHash(rom.cbegin() + logo_offset, rom.cbegin() + logo_offset + 24);
        return header_first_half_hash == logo_first_half_hash;
    } else {
        // The DMG boot ROM checks all 48 bytes, but since we always check the first 24 bytes during cart detection,
        // here we only check the last 24 bytes.
        u32 header_second_half_hash = Fnv1aHash(rom.cbegin() + logo_offset + 24, rom.cbegin() + logo_offset + 48);
        return header_second_half_hash == logo_second_half_hash;
    }
}

} // End namespace Gb
