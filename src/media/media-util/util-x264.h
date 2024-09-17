#pragma once

#include <inttypes.h>

#include "x264/include/x264.h"

#include "public/media-enum.h"

namespace jukey::media::util
{

// Convert x264 pixel format to streamer pixel format
media::PixelFormat ToPixelFormatX264(uint16_t format);

// Convert x264 pixel format to streamer pixel format
uint16_t ToX264PixelFormat(media::PixelFormat format);

}