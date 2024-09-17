#pragma once

#include <vector>

#include "public/media-enum.h"


namespace jukey::media::com
{

//==============================================================================
// Video Capability
//==============================================================================
struct VideoCap
{
	bool operator==(const VideoCap& cap) const
	{
		return (cap.codec == codec) && (cap.format == format) && (cap.res == res);
	}

	bool operator!=(const VideoCap& cap) const
	{
		return (cap.codec != codec) || (cap.format != format) || (cap.res != res);
	}

	media::VideoCodec codec = media::VideoCodec::INVALID;
	media::PixelFormat format = media::PixelFormat::INVALID;
	media::VideoRes res = media::VideoRes::INVALID;
};

//==============================================================================
// Video Capabilities
// vector -> set???
//==============================================================================
struct VideoCaps
{
	void AddCap(media::VideoCodec codec)
	{
		auto iter = std::find(codecs.begin(), codecs.end(), codec);
		if (iter == codecs.end()) {
			codecs.push_back(codec);
		}
	}

	void AddCap(media::PixelFormat format)
	{
		auto iter = std::find(formats.begin(), formats.end(), format);
		if (iter == formats.end()) {
			formats.push_back(format);
		}
	}

	void AddCap(media::VideoRes res)
	{
		auto iter = std::find(ress.begin(), ress.end(), res);
		if (iter == ress.end()) {
			ress.push_back(res);
		}
	}

	std::vector<media::VideoCodec> codecs;
	std::vector<media::PixelFormat> formats;
	std::vector<media::VideoRes> ress;
};

//==============================================================================
// Audio Capability
//==============================================================================
struct AudioCap
{
	bool operator==(const AudioCap& cap) const
	{
		return (cap.codec == codec
			&& cap.chnls == chnls
			&& cap.sbits == sbits
			&& cap.srate == srate);
	}

	media::AudioCodec codec = media::AudioCodec::INVALID;
	media::AudioChnls chnls = media::AudioChnls::INVALID;
	media::AudioSBits sbits = media::AudioSBits::INVALID;
	media::AudioSRate srate = media::AudioSRate::INVALID;
};

//==============================================================================
// Audio Capbilities
//==============================================================================
struct AudioCaps
{
	void AddCap(media::AudioCodec codec)
	{
		auto iter = std::find(codecs.begin(), codecs.end(), codec);
		if (iter == codecs.end()) {
			codecs.push_back(codec);
		}
	}

	void AddCap(media::AudioChnls chnls)
	{
		auto iter = std::find(chnlss.begin(), chnlss.end(), chnls);
		if (iter == chnlss.end()) {
			chnlss.push_back(chnls);
		}
	}

	void AddCap(media::AudioSBits sbits)
	{
		auto iter = std::find(sbitss.begin(), sbitss.end(), sbits);
		if (iter == sbitss.end()) {
			sbitss.push_back(sbits);
		}
	}

	void AddCap(media::AudioSRate srate)
	{
		auto iter = std::find(srates.begin(), srates.end(), srate);
		if (iter == srates.end()) {
			srates.push_back(srate);
		}
	}

	std::vector<media::AudioCodec> codecs;
	std::vector<media::AudioChnls> chnlss;
	std::vector<media::AudioSBits> sbitss;
	std::vector<media::AudioSRate> srates;
};

}
