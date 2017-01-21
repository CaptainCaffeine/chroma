// This file is a part of Chroma.
// Copyright (C) 2016 Matthew Murray
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
#include <cassert>
#include <iostream>

#include "core/CartridgeHeader.h"

namespace Core {

void GetRAMSize(CartridgeHeader& cart_header, const std::vector<u8>& rom) {
    // The RAM size identifier is at 0x0149 in cartridge header.
    switch (rom[0x0149]) {
    case 0x00:
        // Either no external RAM, or MBC2
        cart_header.ram_size = 0x00;
        break;
    case 0x01:
        // 2KB external RAM
        cart_header.ram_size = 0x800;
        break;
    case 0x02:
        // 8KB external RAM
        cart_header.ram_size = 0x2000;
        break;
    case 0x03:
        // 32KB external RAM - 4 banks
        cart_header.ram_size = 0x8000;
        break;
    case 0x04:
        // 128KB external RAM - 16 banks
        cart_header.ram_size = 0x20000;
        break;
    case 0x05:
        // 64KB external RAM - 8 banks
        cart_header.ram_size = 0x10000;
        break;
    default:
        // I don't know if this happens in official games, but it could happen in homebrew.
        assert(false && "Unrecognized external RAM quantity given in cartridge header.");
        break;
    }
}

void GetMBCType(CartridgeHeader& cart_header, const std::vector<u8>& rom) {
    // The MBC type is at 0x0147. The MBC identifier also tells us if this cartridge contains external RAM.
    switch (rom[0x0147]) {
    case 0x00:
        // ROM only, no MBC
        cart_header.mbc_mode = MBC::None;
        cart_header.ext_ram_present = false;
        break;
    case 0x01:
        // MBC1, no RAM
        cart_header.mbc_mode = MBC::MBC1;
        cart_header.ext_ram_present = false;
        break;
    case 0x02:
    case 0x03:
        // MBC1 with external RAM, 0x03 implies the cart has a battery as well.
        cart_header.mbc_mode = MBC::MBC1;
        cart_header.ext_ram_present = true;
        break;
    case 0x05:
        // MBC2, no RAM
        cart_header.mbc_mode = MBC::MBC2;
        cart_header.ext_ram_present = false;
        break;
    case 0x06:
        // MBC2 with embedded nybble RAM
        cart_header.mbc_mode = MBC::MBC2;
        cart_header.ext_ram_present = true;
        break;
    case 0x08:
    case 0x09:
        // ROM + external RAM, no MBC, 0x09 implies battery as well.
        // This is listed in a few cartridge header tables, but Gekkio claims no official games with this
        // configuration exist. (http://gekkio.fi/blog/2015-02-28-mooneye-gb-cartridge-analysis-tetris.html)
        cart_header.mbc_mode = MBC::None;
        cart_header.ext_ram_present = true;
        break;
    case 0x0B:
        // MMM01, no RAM.
        // I can't find any information on this MBC, but it's supposedly present in "Momotarou Collection 2".
        assert(false && "MMM01 unimplemented");
        break;
    case 0x0C:
    case 0x0D:
        // MMM01 with external RAM, 0x0D implies battery as well.
        // I can't find any information on this MBC, but it's supposedly present in "Momotarou Collection 2".
        assert(false && "MMM01 unimplemented");
        break;
    case 0x0F:
    case 0x11:
        // MBC3, no RAM. 0x0F implies timer and battery.
        cart_header.mbc_mode = MBC::MBC3;
        cart_header.ext_ram_present = false;
        break;
    case 0x10:
    case 0x12:
    case 0x13:
        // MBC3 with external RAM. 0x10 implies timer and battery, 0x13 implies battery.
        cart_header.mbc_mode = MBC::MBC3;
        cart_header.ext_ram_present = true;
        break;
    case 0x19:
        // MBC5, no RAM.
        cart_header.mbc_mode = MBC::MBC5;
        cart_header.ext_ram_present = false;
        break;
    case 0x1C:
        // MBC5 with rumble, no RAM.
        cart_header.mbc_mode = MBC::MBC5;
        cart_header.ext_ram_present = false;
        cart_header.rumble_present = true;
        break;
    case 0x1A:
    case 0x1B:
        // MBC5 with external RAM. 0x1B implies battery.
        cart_header.mbc_mode = MBC::MBC5;
        cart_header.ext_ram_present = true;
        break;
    case 0x1D:
    case 0x1E:
        // MBC5 with external RAM and rumble. 0x1E implies battery.
        cart_header.mbc_mode = MBC::MBC5;
        cart_header.ext_ram_present = true;
        cart_header.rumble_present = true;
        break;
    case 0x20:
        // MBC6 with external RAM and battery.
        assert(false && "MBC6 unimplemented");
        break;
    case 0x22:
        // MBC7 with external RAM, battery, and accelerometer. Only used by Kirby Tilt n Tumble.
        assert(false && "MBC7 unimplemented");
        break;
    case 0xFC:
        // Pocket Camera
        assert(false && "Pocket Camera unimplemented");
        break;
    case 0xFD:
        // Bandai TAMA5, used in Tamagotchi games.
        assert(false && "TAMA5 unimplemented");
        break;
    case 0xFE:
        // HuC3 with infrared port
        assert(false && "HuC3 unimplemented");
        break;
    case 0xFF:
        // HuC1 with external RAM, battery, and infrared port
        assert(false && "HuC1 unimplemented");
        break;

    default:
        assert(false && "Unrecognized MBC");
    }
}

void HeaderChecksum(const std::vector<u8>& rom) {
    u8 checksum = 0;
    for (std::size_t i = 0x0134; i < 0x014D; ++i) {
        checksum -= rom[i] + 1;
    }

    // The header checksum at 0x014D must match the value calculated above. This is checked in the boot ROM, and if
    // it does not match the Game Boy locks up.
    if (checksum != rom[0x014D]) {
        std::cerr << "WARNING: Header checksum does not match. This ROM would not run on a Game Boy!" << std::endl;
    }
}

void CheckNintendoLogo(const Console console, const std::vector<u8>& rom) {
    const std::array<u8, 48> logo{{
        0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
        0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
        0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
    }};

    // DMG boot ROM checks all 48 bytes, but the CGB boot ROM only checks the first 24 bytes.
    std::size_t end_addr = 0x0104 + ((console == Console::DMG) ? 48 : 24);
    auto logo_iter = logo.begin();
    for (std::size_t addr = 0x0104; addr < end_addr; ++addr) {
        if (rom[addr] != *logo_iter++) {
            std::cerr << "WARNING: Nintendo logo does not match. This ROM would not run on a Game Boy!" << std::endl;
            break;
        }
    }
}

CartridgeHeader GetCartridgeHeaderInfo(Console& console, const std::vector<u8>& rom) {
    CartridgeHeader cart_header;

    // Determine if this game enables CGB functions. A value of 0xC0 implies the game is CGB-only, and
    // 0x80 implies it can also run on pre-CGB devices. This both have the same effect, as it's up to
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

    if (console == Console::CGB && cgb_flag) {
        cart_header.game_mode = GameMode::CGB;
    } else {
        cart_header.game_mode = GameMode::DMG;
    }

    // The ROM size is at 0x0148 in cartridge header. Each ROM bank is 16KB.
    cart_header.num_rom_banks = (0x8000 << rom[0x0148]) / 0x4000;
    if (rom.size() != cart_header.num_rom_banks*0x4000) {
        std::cerr << "WARNING: Size of provided ROM does not match size given in cartridge header." << std::endl;
    }

    GetRAMSize(cart_header, rom);
    GetMBCType(cart_header, rom);
    CheckNintendoLogo(console, rom);
    HeaderChecksum(rom);

    return cart_header;
}

} // End namespace Core
