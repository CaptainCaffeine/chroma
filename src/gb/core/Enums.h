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

enum class Console {DMG, CGB, AGB, Default};
enum class GameMode {DMG, CGB};
enum class MBC {None, MBC1, MBC1M, MBC2, MBC3, MBC5};

enum class LogLevel {None, Regular, Timer, LCD};

enum class Interrupt : unsigned int {VBLANK=0x01, STAT=0x02, Timer=0x04, Serial=0x08, Joypad=0x10};
