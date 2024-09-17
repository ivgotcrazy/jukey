#include "cap-negotiate.h"
#include "nlohmann/json.hpp"
#include "util-streamer.h"
#include "log.h"

#include "public/media-enum.h"

using json = nlohmann::json;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template<class Param, const Param PARAM_INVALID>
Param NegotiateTemplate(CSTREF key, const json& src_json,
	const std::vector<json>& sinks_json)
{
	LOG_INF("Negotiate template, key:{}", key);
	LOG_INF("\tsrc_json:{}", src_json.dump());

	for (auto sink_json : sinks_json) {
		LOG_INF("\tsink_json:{}", sink_json.dump());
	}

	try {
		std::vector<Param> src_items = src_json[key];
		if (src_items.empty()) {
			throw(std::string("Empty source ").append(key).c_str());
		}

		std::vector<std::vector<Param>> sinks_items;
		for (auto sink_json : sinks_json) {
			sinks_items.push_back(sink_json[key]);
		}

		if (sinks_items.empty()) {
			throw(std::string("Empty sinks ").append(key).c_str());
		}

		for (auto src_item : src_items) {
			uint32_t match_count = 0;
			for (auto sink_items : sinks_items) {
				for (auto sink_item : sink_items) {
					if (src_item == sink_item) {
						match_count++;
						break;
					}
				}
			}
			if (match_count == sinks_items.size()) {
				return src_item;
			}
		}
	}
	catch (const std::exception& e) {
		LOG_ERR("{}", e.what());
		return PARAM_INVALID;
	}

	return PARAM_INVALID;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t NegotiateVideoMaxFrameRate(const json& src_json,
	const std::vector<json>& sinks_json)
{
	try {
		uint32_t src_mfr = src_json["max_frame_rate"];

		std::vector<uint32_t> sinks_mfr;
		for (auto sink_json : sinks_json) {
			sinks_mfr.push_back(sink_json["max_frame_rate"]);
		}

		for (auto sink_mfr : sinks_mfr) {
			if (sink_mfr < src_mfr) {
				src_mfr = sink_mfr;
			}
		}

		return src_mfr;
	}
	catch (const std::exception& e) {
		LOG_ERR("{}", e.what());
		return 0;
	}

	return 0;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::MediaType 
NegotiateMediaType(const json& src_json, const std::vector<json>& sinks_json)
{
	try {
		media::MediaType src_mt = src_json["media_type"];

		std::vector<media::MediaType> sink_mts;
		for (auto sink_json : sinks_json) {
			sink_mts.push_back(sink_json["media_type"]);
		}

		for (auto sink_mt : sink_mts) {
			if (sink_mt != src_mt) {
				return media::MediaType::INVALID;
			}
		}

		return src_mt;
	}
	catch (const std::exception& e) {
		LOG_ERR("{}", e.what());
		return media::MediaType::INVALID;
	}

	return media::MediaType::INVALID;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void NegotiateAudioCap(const json& src_json,
	const std::vector<json>& sinks_json, json& result)
{
	// codec
	media::AudioCodec codec = NegotiateTemplate<media::AudioCodec, 
		media::AudioCodec::INVALID>("codec", src_json, sinks_json);
	if (codec == media::AudioCodec::INVALID) {
		throw("Negotiate audio codec failed!");
	}
	else {
		result["codec"] = codec;
	}

	// channels
	media::AudioChnls chs = NegotiateTemplate<media::AudioChnls,
		media::AudioChnls::INVALID>("channels", src_json, sinks_json);
	if (chs == media::AudioChnls::INVALID) {
		throw("Negotiate audio channels failed!");
	}
	else {
		result["channels"] = chs;
	}

	// Sample bits
	media::AudioSBits sb = NegotiateTemplate<media::AudioSBits,
		media::AudioSBits::INVALID>("sample_bits", src_json, sinks_json);
	if (sb == media::AudioSBits::INVALID) {
		throw("Negotiate audio sample bits failed!");
	}
	else {
		result["sample_bits"] = sb;
	}

	// Sample rate
	media::AudioSRate sr = NegotiateTemplate<media::AudioSRate,
		media::AudioSRate::INVALID>("sample_rate", src_json, sinks_json);
	if (sr == media::AudioSRate::INVALID) {
		throw("Negotiate audio sample rate failed!");
	}
	else {
		result["sample_rate"] = sr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void NegotiateVideoCap(const json& src_json,
	const std::vector<json>& sinks_json, json& result)
{
	// codec
	media::VideoCodec codec = NegotiateTemplate<media::VideoCodec,
		media::VideoCodec::INVALID>("codec", src_json, sinks_json);
	if (codec == media::VideoCodec::INVALID) {
		throw("Negotiate video codec failed!");
	}
	else {
		result["codec"] = codec;
	}

	// resolution
	media::VideoRes res = NegotiateTemplate<media::VideoRes,
		media::VideoRes::INVALID>("resolution", src_json, sinks_json);
	if (res == media::VideoRes::INVALID) {
		throw("Negotiate video resolution failed!");
	}
	else {
		result["resolution"] = res;
	}

	// max frame rate
	//uint32_t mfr = NegotiateVideoMaxFrameRate(src_json, sinks_json);
	//if (mfr == 0) {
	//	throw("Negotiate video max frame rate failed!");
	//}
	//else {
	//	result["max_frame_rate"] = mfr;
	//}

	// pixel format
	media::PixelFormat format = NegotiateTemplate<media::PixelFormat,
		media::PixelFormat::INVALID>("pixel_format", src_json, sinks_json);
	if (format == media::PixelFormat::INVALID) {
		throw("Negotiate video color space failed!");
	}
	else {
		result["pixel_format"] = format;
	}
}

//------------------------------------------------------------------------------
// media_type: video
// codec: []
// resolution: []
// pixel_format: []
// max_frame_rate: 30
//
// media_type: audio
// codec: []
// channels: []
// sample_bits: []
// sample_rate: []
//------------------------------------------------------------------------------
std::string NegotiateCap(CSTREF src, const std::vector<std::string>& sinks)
{
	LOG_INF("Negotiate capability begin ===>");
	LOG_INF("\tSrc caps:{}", src);
	for (auto sink : sinks) {
		LOG_INF("\tSink caps:{}", sink);
	}

	if (src.empty()) {
		LOG_ERR("Emptry src caps!");
		return std::string();
	}

	if (sinks.empty()) {
		LOG_ERR("Empty sinks!");
		return std::string();
	}

	json result;

	try {
		json src_json = json::parse(src);
		std::vector<json> sinks_json;
		for (auto sink : sinks) {
			sinks_json.push_back(json::parse(sink));
		}

		media::MediaType mt = NegotiateMediaType(src_json, sinks_json);
		if (mt == media::MediaType::INVALID) {
			throw("Negotiate media type failed!");
		}
		else {
			result["media_type"] = mt;
		}
		
		if (mt == media::MediaType::AUDIO) {
			NegotiateAudioCap(src_json, sinks_json, result);
		}
		else {
			NegotiateVideoCap(src_json, sinks_json, result);
		}
	}
	catch (const std::exception& e) {
		LOG_ERR("===> {}", e.what());
		return std::string();
	}

	LOG_INF("Negotiate capability result:{}", media::util::Capper(result.dump()));

	return result.dump();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string NegotiateCap(CSTREF src, CSTREF sink)
{
	std::vector<std::string> sinks;
	sinks.push_back(sink);

	return NegotiateCap(src, sinks);
}

}
