#pragma once

#include <inttypes.h>
#include <memory>

#include "media-enum.h"

namespace jukey::media
{

//==============================================================================
// Audio frame
//==============================================================================
struct VideoFramePara
{
	VideoCodec codec  = VideoCodec::INVALID;
	uint16_t   width  = 0;
	uint16_t   height = 0;
	uint32_t   ts     = 0;
	uint32_t   seq    = 0;
	bool       key    = false; // key frame
};
typedef std::shared_ptr<VideoFramePara> VideoFrameParaSP;

//==============================================================================
// Audio frame parameters
// Note: AudioSBits fix to 16bits
//==============================================================================
struct AudioFramePara
{
	AudioCodec codec = AudioCodec::INVALID;
	AudioSRate srate = AudioSRate::INVALID;
	AudioChnls chnls = AudioChnls::INVALID;
	uint32_t   power = 0;
	uint32_t   count = 0; // sample count
	uint32_t   seq   = 0;
	uint32_t   ts    = 0;
};
typedef std::shared_ptr<AudioFramePara> AudioFrameParaSP;

//==============================================================================
// 
//==============================================================================
struct VideoInputDevAttr
{
	std::pair<uint32_t, uint32_t> fps;
	std::vector<media::VideoRes> ress;
	std::vector<media::PixelFormat> formats;
};

//==============================================================================
// 
//==============================================================================
struct AudioInputDevAttr
{
	std::vector<media::AudioSBits> sbitss;
	std::vector<media::AudioSRate> srates;
	std::vector<media::AudioChnls> chnlss;
};

}