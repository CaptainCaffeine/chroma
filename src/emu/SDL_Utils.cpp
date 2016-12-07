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

#include <iostream>

#include "emu/SDL_Utils.h"

namespace Emu {

void PrintSDLError(const std::string& error_function) {
    std::cerr << "SDL_" << error_function << " Error: " << SDL_GetError() << std::endl;
}

int InitSDL(SDLContext& context) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        PrintSDLError("Init");
        return 1;
    }

    context.window = SDL_CreateWindow("Chroma",
                                       SDL_WINDOWPOS_UNDEFINED,
                                       SDL_WINDOWPOS_UNDEFINED,
                                       160,
                                       144,
                                       SDL_WINDOW_SHOWN);
    if (context.window == nullptr) {
        PrintSDLError("CreateWindow");
        SDL_Quit();
        return 1;
    }

    context.renderer = SDL_CreateRenderer(context.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (context.renderer == nullptr) {
        PrintSDLError("CreateRenderer");
        SDL_DestroyWindow(context.window);
        SDL_Quit();
        return 1;
    }

    context.texture = SDL_CreateTexture(context.renderer,
                                        SDL_PIXELFORMAT_RGBA8888,
                                        SDL_TEXTUREACCESS_STREAMING,
                                        160,
                                        144);
    if (context.texture == nullptr) {
        PrintSDLError("CreateTexture");
        SDL_DestroyRenderer(context.renderer);
        SDL_DestroyWindow(context.window);
        SDL_Quit();
        return 1;
    }

    SDL_SetTextureBlendMode(context.texture, SDL_BLENDMODE_NONE);

    return 0;
}

void RenderFrame(const u32* fb_ptr, SDLContext& context) {
    SDL_LockTexture(context.texture, nullptr, &context.texture_pixels, &context.texture_pitch);
    memcpy(context.texture_pixels, fb_ptr, 160*144*sizeof(u32));
    SDL_UnlockTexture(context.texture);

    SDL_RenderClear(context.renderer);
    SDL_RenderCopy(context.renderer, context.texture, nullptr, nullptr);
    SDL_RenderPresent(context.renderer);
}

void CleanupSDL(SDLContext& context) {
    SDL_DestroyTexture(context.texture);
    SDL_DestroyRenderer(context.renderer);
    SDL_DestroyWindow(context.window);
    SDL_Quit();
}

} // End namespace Emu
