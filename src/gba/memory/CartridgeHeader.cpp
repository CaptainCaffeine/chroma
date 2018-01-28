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

#include <array>
#include <iostream>

#include "common/CommonFuncs.h"
#include "gba/memory/Memory.h"

namespace Gba {

bool Memory::CheckNintendoLogo(const std::vector<u8>& rom_header) noexcept {
    // Calculate the FNV-1a hash of the region in the ROM header where the Nintendo logo is supposed to be (0x4-0x9F)
    // and compare it to a precalculated hash of the expected logo.
    static constexpr u32 logo_hash = 0xAF665756;
    u32 header_logo_hash = Fnv1aHash(rom_header.cbegin() + 0x4, rom_header.cbegin() + 0xA0);

    return header_logo_hash == logo_hash;
}

void Memory::CheckHeader(const std::vector<u16>& rom_header) {
    // Fixed value check. All GBA games must have 0x96 stored at 0xB2.
    if (rom_header[0xB2 / 2] != 0x96) {
        std::cerr << "WARNING: Fixed value does not match. This ROM would not run on a GBA!" << std::endl;
    }

    // Header checksum.
    u8 checksum = 0x18;
    for (std::size_t i = 0xA0; i < 0xBD; ++i) {
        checksum += (i % 2) ? (rom_header[i / 2] >> 8) : (rom_header[i / 2] & 0xFF);
    }

    checksum ^= 0xFF;

    if (checksum != (rom_header[0xBD / 2] >> 8)) {
        std::cerr << "WARNING: Header checksum does not match. This ROM would not run on a GBA!" << std::endl;
    }
}

} // End namespace Gba
