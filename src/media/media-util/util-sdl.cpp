#include "util-sdl.h"
#include "log.h"

namespace jukey::media::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SDL_PixelFormatEnum ToSdlPixelFormat(media::PixelFormat format)
{
	switch (format) {
	case media::PixelFormat::YUY2:
		return SDL_PIXELFORMAT_YUY2;
	case media::PixelFormat::NV12:
		return SDL_PIXELFORMAT_NV12;
	case media::PixelFormat::YV12:
		return SDL_PIXELFORMAT_YV12;
	case media::PixelFormat::I420:
		return SDL_PIXELFORMAT_IYUV;
	default:
		LOG_ERR("Unexpected pixel format:{}", format);
		break;
	}

	return SDL_PIXELFORMAT_UNKNOWN;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint16_t ToSdlSampleFormat(media::AudioSBits sbits)
{
	switch (sbits) {
	case media::AudioSBits::S16:
		return AUDIO_S16LSB;
	case media::AudioSBits::S16P: // TODO:
		return AUDIO_S16LSB;
	case media::AudioSBits::S32:
		return AUDIO_S32LSB;
	case media::AudioSBits::S32P: // TODO:
		return AUDIO_S32LSB;
	default:
		LOG_ERR("Unexpected audio sample bits:{}", sbits);
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t GetBitsPerPixel(media::PixelFormat format)
{
	switch (format) {
	case media::PixelFormat::YUY2: // 4:2:2
		return 16;
	case media::PixelFormat::NV12: // 4:2:0
		return 12;
	case media::PixelFormat::YV12: // 4:2:0
		return 12;
	case media::PixelFormat::I420: // 4:2:0
		return 12;
	case media::PixelFormat::RGB24:
		return 24;
	default:
		LOG_ERR("Unexpected pixel format:{}", format);
		break;
	}

	return 0;
}

}