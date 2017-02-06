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

#include <string>
#include <stdexcept>

#include "emu/SDL_Utils.h"

namespace Emu {

const std::string GetSDLErrorString(const std::string& error_function) {
    return {"SDL_" + error_function + " Error: " + SDL_GetError()};
}

void InitSDL(SDLContext& context, unsigned int scale, bool fullscreen) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(GetSDLErrorString("Init"));
    }

    context.window = SDL_CreateWindow("Chroma",
                                       SDL_WINDOWPOS_UNDEFINED,
                                       SDL_WINDOWPOS_UNDEFINED,
                                       160*scale,
                                       144*scale,
                                       SDL_WINDOW_SHOWN);
    if (context.window == nullptr) {
        SDL_Quit();
        throw std::runtime_error(GetSDLErrorString("CreateWindow"));
    }

    context.renderer = SDL_CreateRenderer(context.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (context.renderer == nullptr) {
        SDL_DestroyWindow(context.window);
        SDL_Quit();
        throw std::runtime_error(GetSDLErrorString("CreateRenderer"));
    }

    SDL_RenderSetLogicalSize(context.renderer, 160, 144);
    SDL_RenderSetIntegerScale(context.renderer, SDL_TRUE);

    context.texture = SDL_CreateTexture(context.renderer,
                                        SDL_PIXELFORMAT_ABGR1555,
                                        SDL_TEXTUREACCESS_STREAMING,
                                        160,
                                        144);
    if (context.texture == nullptr) {
        SDL_DestroyRenderer(context.renderer);
        SDL_DestroyWindow(context.window);
        SDL_Quit();
        throw std::runtime_error(GetSDLErrorString("CreateTexture"));
    }

    SDL_SetTextureBlendMode(context.texture, SDL_BLENDMODE_NONE);

    if (fullscreen) {
        SDL_SetWindowFullscreen(context.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

void RenderFrame(const u16* fb_ptr, SDLContext& context) {
    SDL_LockTexture(context.texture, nullptr, &context.texture_pixels, &context.texture_pitch);
    memcpy(context.texture_pixels, fb_ptr, 160*144*sizeof(u16));
    SDL_UnlockTexture(context.texture);

    SDL_RenderClear(context.renderer);
    SDL_RenderCopy(context.renderer, context.texture, nullptr, nullptr);
    SDL_RenderPresent(context.renderer);
}

void ToggleFullscreen(SDLContext& context) {
    u32 fullscreen = SDL_GetWindowFlags(context.window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_SetWindowFullscreen(context.window, fullscreen ^ SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void CleanupSDL(SDLContext& context) {
    SDL_DestroyTexture(context.texture);
    SDL_DestroyRenderer(context.renderer);
    SDL_DestroyWindow(context.window);
    SDL_Quit();
}

} // End namespace Emu
