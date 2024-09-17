#pragma once

#include "public/media-enum.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/pixfmt.h"
}

namespace jukey::media::util
{

// Convert ffmpeg video codec to streamer video codec
media::VideoCodec ToVideoCodecFF(AVCodecID codec);

// Convert ffmpeg pixel format to streamer pixel format
media::PixelFormat ToPixelFormatFF(AVPixelFormat format);


// Convert streamer audio codec to ffmpeg audio codec
AVCodecID ToFfAudioCodec(media::AudioCodec codec);

// Convert streamer video codec to ffmpeg video codec
AVCodecID ToFfVideoCodec(media::VideoCodec codec);

// Convert streamer pixel format to ffmpeg pixel format
AVPixelFormat ToFfPixelFormat(media::PixelFormat format);

// Convert streamer sample format to ffmpeg sample format (audio)
AVSampleFormat ToFfSampleFormat(media::AudioSBits sbits);


// Convert ffmpeg audio codec to streamer audio codec
media::AudioCodec ToAudioCodecFF(AVCodecID codec);

// Convert audio sample bits to streamer enum
media::AudioSBits ToAudioSBitsFF(int format);

// Convert audio sample rate to streamer enum
media::AudioSRate ToAudioSRateFF(int sample_rate);

// Convert audio channels to streamer enum
media::AudioChnls ToAudioChnlsFF(int channels);

}