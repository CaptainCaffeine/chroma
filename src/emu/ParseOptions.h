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

#pragma once

#include <string>
#include <vector>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"

namespace Emu {

std::vector<std::string> GetTokens(char** begin, char** end);
bool ContainsOption(const std::vector<std::string>& tokens, const std::string& option);
std::string GetOptionParam(const std::vector<std::string>& tokens, const std::string& option);

void DisplayHelp();
Console GetGameBoyType(const std::vector<std::string>& tokens);
LogLevel GetLogLevel(const std::vector<std::string>& tokens);
unsigned int GetPixelScale(const std::vector<std::string>& tokens);

std::string SaveGamePath(const std::string& rom_path);
std::vector<u8> LoadROM(const std::string& filename);
std::vector<u8> LoadSaveGame(const std::string& filename);
void CheckPathIsRegularFile(const std::string& filename);

} // End namespace Emu
