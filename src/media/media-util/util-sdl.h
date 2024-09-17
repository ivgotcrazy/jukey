#pragma once

#include "public/media-enum.h"

extern "C"
{
#include "SDL.h"
#include "SDL_pixels.h"
}

namespace jukey::media::util
{

/**
 * Pixel format convert
 */
SDL_PixelFormatEnum ToSdlPixelFormat(media::PixelFormat format);

uint16_t ToSdlSampleFormat(media::AudioSBits sbits);

uint32_t GetBitsPerPixel(media::PixelFormat format);

}