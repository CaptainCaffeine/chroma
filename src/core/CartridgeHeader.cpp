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

#include <cassert>
#include <iostream>

#include "core/CartridgeHeader.h"

namespace Core {

CartridgeHeader GetCartridgeHeaderInfo(const Console console, const std::vector<u8>& rom) {
    CartridgeHeader cart_header;

    // Determine if this game enables CGB functions. A value of 0xC0 implies the game is CGB-only, and 
    // 0x80 implies it can also run on pre-CGB devices. This both have the same effect, as it's up to 
    // the game to test if it is running on a pre-CGB device.
    if (console == Console::CGB && (rom[0x0143] == 0xC0 || rom[0x0143] == 0x80)) {
        cart_header.game_mode = GameMode::CGB;
    } else {
        cart_header.game_mode = GameMode::DMG;
    }

    // The ROM size is at 0x0148 in cartridge header. Each ROM bank is 16KB.
    cart_header.num_rom_banks = (0x8000 << rom[0x0148]) / 0x4000;
    if (rom.size() != cart_header.num_rom_banks*0x4000) {
        std::cerr << "WARNING: Size of provided ROM does not match size given in cartridge header." << std::endl;
    }

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
        assert(false && "MBC3 unimplemented");
        break;
    case 0x10:
    case 0x12:
    case 0x13:
        // MBC3 with external RAM. 0x10 implies timer and battery, 0x13 implies battery.
        assert(false && "MBC3 unimplemented");
        break;
    case 0x19:
    case 0x1C:
        // MBC5, no RAM. 0x1C implies rumble.
        assert(false && "MBC5 unimplemented");
        break;
    case 0x1A:
    case 0x1B:
    case 0x1D:
    case 0x1E:
        // MBC5 with external RAM. 0x1B implies battery, 0x1D implies rumble, and 0x1E implies battery and rumble.
        assert(false && "MBC5 unimplemented");
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

    return cart_header;
}

} // End namespace Core