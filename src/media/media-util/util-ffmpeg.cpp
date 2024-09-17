#include "util-ffmpeg.h"
#include "log.h"

namespace jukey::media::util
{
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::AudioCodec ToAudioCodecFF(AVCodecID codec)
{
	switch (codec) {
	case AV_CODEC_ID_AAC:
		return media::AudioCodec::AAC;
	case AV_CODEC_ID_OPUS:
		return media::AudioCodec::OPUS;
	default:
		LOG_ERR("Invalid audio codec:{}", codec);
		return media::AudioCodec::INVALID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AVCodecID ToFfAudioCodec(media::AudioCodec codec)
{
	switch (codec) {
	case media::AudioCodec::AAC:
		return AV_CODEC_ID_AAC;
	case media::AudioCodec::OPUS:
		return AV_CODEC_ID_OPUS;
	default:
		LOG_ERR("Invalid audio codec:{}", codec);
		return AV_CODEC_ID_NONE;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::VideoCodec ToVideoCodecFF(AVCodecID codec)
{
	switch (codec) {
	case AV_CODEC_ID_H264:
		return media::VideoCodec::H264;
	default:
		LOG_ERR("Invalid video codec:{}", codec);
		return media::VideoCodec::INVALID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AVCodecID ToFfVideoCodec(media::VideoCodec codec)
{
	switch (codec) {
	case media::VideoCodec::H264:
		return AV_CODEC_ID_H264;
	default:
		LOG_ERR("Invalid video codec:{}", codec);
		return AV_CODEC_ID_NONE;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::PixelFormat ToPixelFormatFF(AVPixelFormat format)
{
	switch (format) {
	case AV_PIX_FMT_YUV420P:
		return media::PixelFormat::I420;
	default:
		LOG_ERR("Invalid pixel format:{}", format);
		return media::PixelFormat::INVALID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AVPixelFormat ToFfPixelFormat(media::PixelFormat format)
{
	switch (format) {
	case media::PixelFormat::I420:
		return AV_PIX_FMT_YUV420P;
	case media::PixelFormat::NV12:
		return AV_PIX_FMT_NV12;
	case media::PixelFormat::YUY2:
		return AV_PIX_FMT_YUYV422;
	case media::PixelFormat::RGB24:
		return AV_PIX_FMT_RGB24;
	default:
		LOG_ERR("Invalid pixel format:{}", format);
		return AV_PIX_FMT_NONE;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AVSampleFormat ToFfSampleFormat(media::AudioSBits sbits)
{
	switch (sbits) {
	case media::AudioSBits::S16:
		return AV_SAMPLE_FMT_S16;
	case media::AudioSBits::S32:
		return AV_SAMPLE_FMT_S32;
	case media::AudioSBits::S16P:
		return AV_SAMPLE_FMT_S16P;
	case media::AudioSBits::S32P:
		return AV_SAMPLE_FMT_S32P;
	case media::AudioSBits::FLTP:
		return AV_SAMPLE_FMT_FLTP;
	default:
		LOG_ERR("Invalid audio sample bits:{}", sbits);
		return AV_SAMPLE_FMT_NONE;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::AudioSBits ToAudioSBitsFF(int format)
{
	if (format == AV_SAMPLE_FMT_S16) {
		return media::AudioSBits::S16;
	}
	else if (format == AV_SAMPLE_FMT_S32) {
		return media::AudioSBits::S32;
	}
	else if (format == AV_SAMPLE_FMT_S16P) {
		return media::AudioSBits::S16P;
	}
	else if (format == AV_SAMPLE_FMT_S32P) { 
		return media::AudioSBits::S32P;
	}
	else if (format == AV_SAMPLE_FMT_FLTP) {
		return media::AudioSBits::FLTP;
	}
	else if (format == AV_SAMPLE_FMT_DBLP) {
		return media::AudioSBits::DBLP;
	}
	else {
		LOG_ERR("Invalid FF sample format:{}", format);
		return media::AudioSBits::INVALID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::AudioSRate ToAudioSRateFF(int sample_rate)
{
	if (sample_rate == 16000) {
		return media::AudioSRate::SR_16K;
	}
	else if (sample_rate == 48000) {
		return media::AudioSRate::SR_48K;
	}
	else {
		LOG_ERR("Invalid sample rate:{}", sample_rate);
		return media::AudioSRate::INVALID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::AudioChnls ToAudioChnlsFF(int channels)
{
	if (channels == 1) {
		return media::AudioChnls::MONO;
	}
	else if (channels == 2) {
		return media::AudioChnls::STEREO;
	}
	else {
		LOG_ERR("Invalid audio channels:{}", channels);
		return media::AudioChnls::INVALID;
	}
}

}