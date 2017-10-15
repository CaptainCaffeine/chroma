// This file is a part of Chroma.
// Copyright (C) 2016-2017 Matthew Murray
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
#include "gb/core/Enums.h"

namespace Gb {

class CartridgeHeader {
public:
    CartridgeHeader(Console& console, const std::vector<u8>& rom, bool multicart_requested);

    GameMode game_mode;
    MBC mbc_mode;
    bool ext_ram_present;
    unsigned int ram_size;
    unsigned int num_rom_banks;
    bool rtc_present = false;
    bool rumble_present = false;
private:
    void GetRAMSize(const std::vector<u8>& rom);
    void GetMBCType(const std::vector<u8>& rom);
    void HeaderChecksum(const std::vector<u8>& rom) const;
    void CheckNintendoLogo(const Console console, const std::vector<u8>& rom) const;
};

} // End namespace Gb
