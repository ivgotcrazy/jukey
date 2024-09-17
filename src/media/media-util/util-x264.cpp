#include "util-x264.h"
#include "log.h"

namespace jukey::media::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::PixelFormat ToPixelFormatX264(uint16_t format)
{
	switch (format) {
	case X264_CSP_I420:
		return media::PixelFormat::I420;
	default:
		LOG_ERR("Unsupport x264 format:{}", format);
		return media::PixelFormat::INVALID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint16_t ToX264PixelFormat(media::PixelFormat format)
{
	switch (format) {
	case media::PixelFormat::I420:
		return X264_CSP_I420;
	case media::PixelFormat::NV12:
		return X264_CSP_NV12;
	case media::PixelFormat::YUY2:
		return X264_CSP_YUYV;
	case media::PixelFormat::RGB24:
		return X264_CSP_RGB;
	default:
		LOG_ERR("Unsupport streamer format:{}", format);
		return X264_CSP_NONE;
	}
}

}