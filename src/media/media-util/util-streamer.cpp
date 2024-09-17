#include "util-streamer.h"
#include "util-enum.h"
#include "nlohmann/json.hpp"
#include "common/util-common.h"
#include "log.h"

using json = nlohmann::json;

namespace jukey::media::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t GetWidth(media::VideoRes resolution)
{
	switch (resolution) {
	case media::VideoRes::RES_640x360:
		return 640;
	case media::VideoRes::RES_640x480:
		return 640;
	case media::VideoRes::RES_1280x720:
		return 1280;
	case media::VideoRes::RES_1920x1080:
		return 1920;
	default:
		LOG_ERR("Invalid resolution:{}", resolution);
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t GetHeight(media::VideoRes resolution)
{
	switch (resolution) {
	case media::VideoRes::RES_640x360:
		return 360;
	case media::VideoRes::RES_640x480:
		return 480;
	case media::VideoRes::RES_1280x720:
		return 720;
	case media::VideoRes::RES_1920x1080:
		return 1080;
	default:
		LOG_ERR("Invalid resolution:{}", resolution);
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::VideoRes ToVideoRes(uint32_t width, uint32_t height)
{
	if (width == 1920 && height == 1080) {
		return media::VideoRes::RES_1920x1080;
	}
	else if (width == 1280 && height == 720) {
		return media::VideoRes::RES_1280x720;
	}
	else if (width == 640 && height == 480) {
		return media::VideoRes::RES_640x480;
	}
	else if (width == 640 && height == 360) {
		return media::VideoRes::RES_640x360;
	}
	else if (width == 320 && height == 240) {
		return media::VideoRes::RES_320x240;
	}
	else if (width == 320 && height == 180) {
		return media::VideoRes::RES_320x180;
	}
	else {
		LOG_ERR("Unsupport width:{}, height:{}", width, height);
		return media::VideoRes::INVALID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t GetLineBytes(media::VideoRes res, media::PixelFormat format)
{
	uint32_t width = GetWidth(res);

	switch (format) {
	case media::PixelFormat::YUY2:
		return width * 2;
	case media::PixelFormat::NV12:
		return width * 3 / 2;
	default:
		LOG_ERR("Unknown color space:{}", format);
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::PinDataSP ClonePinData(const stmr::PinData& data)
{
	stmr::PinDataSP new_data(new stmr::PinData());

	// Copy properties
	new_data->mt  = data.mt;
	new_data->pts = data.pts;
	new_data->dts = data.dts;
	new_data->pos = data.pos;
	new_data->drt = data.drt;
	new_data->syn = data.syn;
	new_data->tbd = data.tbd;
	new_data->tbn = data.tbn;

	// TODO: parameters without copy
	new_data->media_para = data.media_para;

	// Copy data
	for (auto i = 0; i < 8; i++) {
		uint32_t i_len = data.media_data[i].data_len;
		if (i_len > 0) {
			new_data->media_data[i].data.reset(new uint8_t[i_len]);
			new_data->media_data[i].data_len = i_len;
			new_data->media_data[i].total_len = i_len;
			memcpy(DP(new_data->media_data[i]), DP(data.media_data[i]), i_len);
		}
	}

	new_data->data_count = data.data_count;

	return new_data;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t GetSRateNum(media::AudioSRate srate)
{
	switch (srate) {
	case media::AudioSRate::SR_8K:
		return 8000;
	case media::AudioSRate::SR_16K:
		return 16000;
	case media::AudioSRate::SR_48K:
		return 48000;
	default:
		LOG_ERR("Unexpected sample rate:{}", srate);
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::AudioSRate ToAudioSRate(uint32_t srate)
{
	switch (srate) {
	case 8000:
		return media::AudioSRate::SR_8K;
	case 16000:
		return media::AudioSRate::SR_16K;
	case 48000:
		return media::AudioSRate::SR_48K;
	default:
		LOG_ERR("Invalid sample rate:{}", srate);
		return media::AudioSRate::INVALID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::AudioSRate ToAudioSRateProt(uint8_t srate)
{
	switch (srate) {
	case 0:
		return media::AudioSRate::SR_8K;
	case 1:
		return media::AudioSRate::SR_16K;
	case 2:
		return media::AudioSRate::SR_48K;
	default:
		LOG_ERR("Invalid srate:{}", srate);
		return media::AudioSRate::INVALID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint8_t ToProtAudioSRate(media::AudioSRate srate)
{
	switch (srate) {
	case media::AudioSRate::SR_8K:
		return 0;
	case media::AudioSRate::SR_16K:
		return 1;
	case media::AudioSRate::SR_48K:
		return 2;
	default:
		LOG_ERR("Invalid srate:{}", srate);
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t GetChnlsNum(media::AudioChnls chnls)
{
	switch (chnls) {
	case media::AudioChnls::MONO:
		return 1;
	case media::AudioChnls::STEREO:
		return 2;
	default:
		LOG_ERR("Unexpected channels:{}", chnls);
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::AudioChnls ToAudioChnls(uint32_t chnls)
{
	switch (chnls) {
	case 1:
		return media::AudioChnls::MONO;
	case 2:
		return media::AudioChnls::STEREO;
	default:
		LOG_ERR("Invalid channle count:{}", chnls);
		return media::AudioChnls::INVALID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::AudioChnls ToAudioChnlsProt(uint8_t chnls)
{
	switch (chnls) {
	case 0:
		return media::AudioChnls::MONO;
	case 1:
		return media::AudioChnls::STEREO;
	default:
		LOG_ERR("Invalid channel count:{}", chnls);
		return media::AudioChnls::INVALID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint8_t ToProtAudioChnls(media::AudioChnls chnls)
{
	switch (chnls) {
	case media::AudioChnls::MONO:
		return 0;
	case media::AudioChnls::STEREO:
		return 1;
	default:
		LOG_ERR("Invalid channel count:{}", chnls);
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t GetSBitsNum(media::AudioSBits sbits)
{
	switch (sbits) {
	case media::AudioSBits::S16:
		return 16;
	case media::AudioSBits::S32:
		return 32;
	case media::AudioSBits::S16P:
		return 16;
	case media::AudioSBits::S32P:
		return 32;
	case media::AudioSBits::FLTP:
		return 32;
	case media::AudioSBits::DBLP:
		return 64;
	default:
		LOG_ERR("Unexpected sample bits:{}", sbits);
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::AudioSBits ToAudioSBits(uint32_t sbits)
{
	switch (sbits) {
	case 16:
		return media::AudioSBits::S16;
	case 32:
		return media::AudioSBits::S32;
	default:
		LOG_ERR("Invalid sample bits:{}", sbits);
		return media::AudioSBits::INVALID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool IsPlanar(media::AudioSBits sbits)
{
	switch (sbits) {
	case media::AudioSBits::S16P:
		return true;
	case media::AudioSBits::S32P:
		return true;
	case media::AudioSBits::FLTP:
		return true;
	case media::AudioSBits::DBLP:
		return true;
	default:
		return false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool IsInvalidVideoCap(media::com::VideoCap vcap)
{
	return (vcap.codec == media::VideoCodec::INVALID
		|| vcap.format == media::PixelFormat::INVALID
		|| vcap.res == media::VideoRes::INVALID);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool InInvalidAudioCap(media::com::AudioCap acap)
{
	return (acap.codec == media::AudioCodec::INVALID
		|| acap.chnls == media::AudioChnls::INVALID
		|| acap.sbits == media::AudioSBits::INVALID
		|| acap.srate == media::AudioSRate::INVALID);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string ToVideoCapStr(const media::com::VideoCap& cap)
{
  try {
		json cap_json;
		cap_json["media_type"] = media::MediaType::VIDEO;
		cap_json["codec"] = cap.codec;
		cap_json["resolution"] = cap.res;
		cap_json["pixel_format"] = cap.format;

		return cap_json.dump();
  }
  catch (const std::exception& e) {
		LOG_ERR("Call ToVideoCapStr failed, error:{}", e.what());
		return "";
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string ToVideoCapRealStr(const media::com::VideoCap& cap)
{
  try {
		json cap_json;
		cap_json["media_type"] = MEDIA_TYPE_STR(media::MediaType::VIDEO);
		cap_json["codec"] = VIDEO_CODEC_STR(cap.codec);
		cap_json["resolution"] = VIDEO_RES_STR(cap.res);
		cap_json["pixel_format"] = VIDEO_FMT_STR(cap.format);

		return cap_json.dump();
  }
  catch (const std::exception& e) {
		LOG_ERR("Call ToVideoCapStr failed, error:{}", e.what());
		return "";
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string ToVideoCapsRealStr(const media::com::VideoCaps& caps)
{
  try {
		json caps_json;

		caps_json["media_type"] = MEDIA_TYPE_STR(media::MediaType::VIDEO);

		std::string codecs("[");
		for (auto& codec : caps.codecs) {
			codecs.append(VIDEO_CODEC_STR(codec)).append(",");
		}
		codecs.append("]");
		caps_json["codec"] = codecs;

		std::string ress("[");
		for (auto& res : caps.ress) {
			ress.append(VIDEO_RES_STR(res)).append(",");
		}
		ress.append("]");
		caps_json["resolution"] = ress;

		std::string formats("[");
		for (auto& format : caps.formats) {
			formats.append(VIDEO_FMT_STR(format)).append(",");
		}
		formats.append("]");
		caps_json["pixel_format"] = formats;

		return caps_json.dump();
  }
  catch (const std::exception& e) {
		LOG_ERR("Call ToVideoCapsRealStr failed, error:{}", e.what());
		return "";
  }
}

//------------------------------------------------------------------------------
// Capatility act as capabilities
//------------------------------------------------------------------------------
std::string ToVideoCapsStr(const media::com::VideoCap& cap)
{
  media::com::VideoCaps caps;
  caps.AddCap(cap.codec);
  caps.AddCap(cap.format);
  caps.AddCap(cap.res);

  return ToVideoCapsStr(caps);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string ToVideoCapsStr(const media::com::VideoCaps& caps)
{
  try {
		json cap_json;
		cap_json["media_type"] = media::MediaType::VIDEO;
		cap_json["codec"] = caps.codecs;
		cap_json["resolution"] = caps.ress;
		cap_json["pixel_format"] = caps.formats;

		return cap_json.dump();
  }
  catch (const std::exception& e) {
		LOG_ERR("Call ToVideoCapsStr failed, error:{}", e.what());
		return "";
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParseVideoCap(CSTREF from, media::com::VideoCap& to)
{
  try {
		auto result = json::parse(from);
		to.codec = result["codec"].get<media::VideoCodec>();
		to.res = result["resolution"].get<media::VideoRes>();
		to.format = result["pixel_format"].get<media::PixelFormat>();

		return true;
  }
  catch (const std::exception& e) {
		LOG_ERR("Call ParseVideoCap failed, from:{}, error:{}", from, e.what());
		return false;
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<media::com::VideoCap> ParseVideoCap(CSTREF cap_str)
{
  media::com::VideoCap vcap;
  try {
		auto result = json::parse(cap_str);
		vcap.codec = result["codec"].get<media::VideoCodec>();
		vcap.res = result["resolution"].get<media::VideoRes>();
		vcap.format = result["pixel_format"].get<media::PixelFormat>();

		return vcap;
  }
  catch (const std::exception& e) {
		LOG_ERR("ParseVideoCap failed, str:{}, error:{}", cap_str, e.what());
		return std::nullopt;
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<media::com::VideoCaps> ParseVideoCaps(CSTREF caps_str)
{
	media::com::VideoCaps caps;
  try {
		auto result = json::parse(caps_str);

		caps.codecs = result["codec"].get<std::vector<media::VideoCodec>>();
		if (caps.codecs.empty()) {
			LOG_ERR("Empty video codecs!");
			return std::nullopt;
		}

		caps.ress = result["resolution"].get<std::vector<media::VideoRes>>();
		if (caps.ress.empty()) {
			LOG_ERR("Empty video ress!");
			return std::nullopt;
		}

		caps.formats = result["pixel_format"].get<std::vector<media::PixelFormat>>();
		if (caps.formats.empty()) {
			LOG_ERR("Empty video formats!");
			return std::nullopt;
		}

		return caps;
  }
  catch (const std::exception& e) {
		LOG_ERR("ParseVideoCaps failed, caps:{}, err:{}", caps_str, e.what());
		return std::nullopt;
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string ToAudioCapStr(const media::com::AudioCap& cap)
{
  try {
		json cap_json;
		cap_json["media_type"] = media::MediaType::AUDIO;
		cap_json["codec"] = cap.codec;
		cap_json["channels"] = cap.chnls;
		cap_json["sample_bits"] = cap.sbits;
		cap_json["sample_rate"] = cap.srate;

		return cap_json.dump();
  }
  catch (const std::exception& e) {
		LOG_ERR("Call ToAudioCapStr failed, error:{}", e.what());
		return "";
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string ToAudioCapRealStr(const media::com::AudioCap& cap)
{
  try {
		json cap_json;

		cap_json["media_type"] = MEDIA_TYPE_STR(media::MediaType::AUDIO);
		cap_json["codec"] = AUDIO_CODEC_STR(cap.codec);
		cap_json["channels"] = AUDIO_CHNLS_STR(cap.chnls);
		cap_json["sample_bits"] = AUDIO_SBITS_STR(cap.sbits);
		cap_json["sample_rate"] = AUDIO_SRATE_STR(cap.srate);

		return cap_json.dump();
  }
  catch (const std::exception& e) {
		LOG_ERR("Call ToAudioCapStr failed, error:{}", e.what());
		return "";
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string ToAudioCapsRealStr(const media::com::AudioCaps& caps)
{
  try {
		json caps_json;

		caps_json["media_type"] = MEDIA_TYPE_STR(media::MediaType::AUDIO);

		std::string codecs("[");
		for (auto& codec : caps.codecs) {
			codecs.append(AUDIO_CODEC_STR(codec)).append(",");
		}
		codecs.append("]");
		caps_json["codec"] = codecs;

		std::string chnlss("[");
		for (auto& chnls : caps.chnlss) {
			chnlss.append(AUDIO_CHNLS_STR(chnls)).append(",");
		}
		chnlss.append("]");
		caps_json["channels"] = chnlss;

		std::string sbitss("[");
		for (auto& sbits : caps.sbitss) {
			sbitss.append(AUDIO_SBITS_STR(sbits)).append(",");
		}
		sbitss.append("]");
		caps_json["sample_bits"] = sbitss;

		std::string srates("[");
		for (auto& srate : caps.srates) {
			srates.append(AUDIO_SRATE_STR(srate)).append(",");
		}
		srates.append("]");
		caps_json["sample_rate"] = srates;

		return caps_json.dump();
  }
  catch (const std::exception& e) {
		LOG_ERR("Call ToAudioCapsRealStr failed, error:{}", e.what());
		return "";
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string ToAudioCapsStr(const media::com::AudioCaps& caps)
{
  try {
		json cap_json;
		cap_json["media_type"] = media::MediaType::AUDIO;
		cap_json["codec"] = caps.codecs;
		cap_json["channels"] = caps.chnlss;
		cap_json["sample_bits"] = caps.sbitss;
		cap_json["sample_rate"] = caps.srates;

		return cap_json.dump();
  }
  catch (const std::exception& e) {
		LOG_ERR("Call ToAudioCapsStr failed, error:{}", e.what());
		return "";
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string ToAudioCapsStr(const media::com::AudioCap& cap)
{
  media::com::AudioCaps caps;
  caps.AddCap(cap.codec);
  caps.AddCap(cap.chnls);
  caps.AddCap(cap.sbits);
  caps.AddCap(cap.srate);

  return ToAudioCapsStr(caps);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<media::com::AudioCap> media::util::ParseAudioCap(CSTREF cap_str)
{
  media::com::AudioCap cap;
  try {
		auto result = json::parse(cap_str);
		cap.codec = result["codec"].get<media::AudioCodec>();
		cap.chnls = result["channels"].get<media::AudioChnls>();
		cap.sbits = result["sample_bits"].get<media::AudioSBits>();
		cap.srate = result["sample_rate"].get<media::AudioSRate>();

		return cap;
  }
  catch (const std::exception& e) {
		LOG_ERR("ParseAudioCap failed, from:{}, error:{}", cap_str, e.what());
		return std::nullopt;
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<media::com::AudioCaps> ParseAudioCaps(CSTREF caps_str)
{
  media::com::AudioCaps caps;
  try {
		auto result = json::parse(caps_str);

		caps.codecs = result["codec"].get<std::vector<media::AudioCodec>>();
		if (caps.codecs.empty()) {
			LOG_ERR("Empty audio codecs!");
			return std::nullopt;
		}

		caps.chnlss = result["channels"].get<std::vector<media::AudioChnls>>();
		if (caps.chnlss.empty()) {
			LOG_ERR("Empty audio chnlss!");
			return std::nullopt;
		}

		caps.sbitss = result["sample_bits"].get<std::vector<media::AudioSBits>>();
		if (caps.sbitss.empty()) {
			LOG_ERR("Empty audio sbitss!");
			return std::nullopt;
		}

		caps.srates = result["sample_rate"].get<std::vector<media::AudioSRate>>();
		if (caps.srates.empty()) {
			LOG_ERR("Empty audio srates!");
			return std::nullopt;
		}

		return caps;
  }
  catch (const std::exception& e) {
		LOG_ERR("ParseAudioCaps failed, from:{}, erro:{}", caps_str, e.what());
		return std::nullopt;
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string media::util::Capper(CSTREF cap)
{
  if (cap.empty()) {
		return std::string("[]");
  }

  try {
		json cap_json = json::parse(cap);

		media::MediaType mt = cap_json["media_type"];

		if (mt == media::MediaType::AUDIO) {
			std::optional<media::com::AudioCap> ocap = media::util::ParseAudioCap(cap);
			if (ocap.has_value()) {
				return ToAudioCapRealStr(ocap.value());
			}
			else {
				throw("Parse audio cap failed!");
			}
		}
		else if (mt == media::MediaType::VIDEO) {
			std::optional<media::com::VideoCap> ocap = media::util::ParseVideoCap(cap);
			if (ocap.has_value()) {
				return ToVideoCapRealStr(ocap.value());
			}
			else {
				throw("Parse video cap failed!");
			}
		}
		else {
			throw("Invalid media type!");
		}
  }
  catch (const std::exception& e) {
		LOG_ERR("Error:{}", cap, e.what());
		return std::string();
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string Cappers(CSTREF caps)
{
  if (caps.empty()) {
		return std::string("[]");
  }

  try {
		json cap_json = json::parse(caps);

		media::MediaType mt = cap_json["media_type"];

		if (mt == media::MediaType::AUDIO) {
			std::optional<media::com::AudioCaps> ocaps = ParseAudioCaps(caps);
			if (ocaps.has_value()) {
				return ToAudioCapsRealStr(ocaps.value());
			}
			else {
				throw("Parse audio cap failed!");
			}
		}
		else if (mt == media::MediaType::VIDEO) {
			std::optional<media::com::VideoCaps> ocaps = ParseVideoCaps(caps);
			if (ocaps.has_value()) {
				return ToVideoCapsRealStr(ocaps.value());
			}
			else {
				throw("Parse video cap failed!");
			}
		}
		else {
			throw("Invalid media type!");
		}
  }
  catch (const std::exception& e) {
		LOG_ERR("Error:{}", caps, e.what());
		return std::string();
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
jukey::com::ErrCode ParseAvaiAudioCaps(const std::string& caps_str,
  std::vector<std::string>& caps)
{
  std::optional<media::com::AudioCaps> ocaps = util::ParseAudioCaps(caps_str);
  if (!ocaps.has_value()) {
		LOG_ERR("Invalid caps:{}", caps_str);
		return jukey::com::ERR_CODE_INVALID_PARAM;
  }

	media::com::AudioCap cap;
  for (auto& codec : ocaps.value().codecs) {
		cap.codec = codec;
		for (auto& chnls : ocaps.value().chnlss) {
			cap.chnls = chnls;
			for (auto& sbits : ocaps.value().sbitss) {
				cap.sbits = sbits;
				for (auto& srate : ocaps.value().srates) {
					cap.srate = srate;
					caps.push_back(util::ToAudioCapStr(cap));
				}
			}
		}
  }

  return jukey::com::ErrCode::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
jukey::com::ErrCode ParseAvaiVideoCaps(const std::string& caps_str,
  std::vector<std::string>& caps)
{
  std::optional<media::com::VideoCaps> ocaps = util::ParseVideoCaps(caps_str);
  if (!ocaps.has_value()) {
		LOG_ERR("Invalid caps:{}", caps_str);
		return jukey::com::ERR_CODE_INVALID_PARAM;
  }

	media::com::VideoCap cap;
  for (auto& codec : ocaps.value().codecs) {
		cap.codec = codec;
		for (auto& res : ocaps.value().ress) {
			cap.res = res;
			for (auto& format : ocaps.value().formats) {
				cap.format = format;
				caps.push_back(util::ToVideoCapStr(cap));
			}
		}
  }

  return jukey::com::ErrCode::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MatchAudioCaps(const std::string& cap, const std::string& caps)
{
  std::optional<media::com::AudioCap> ocap = util::ParseAudioCap(cap);
  if (!ocap.has_value()) {
		LOG_ERR("Parse audio cap:{} failed!", cap);
		return false;
  }

  std::optional<media::com::AudioCaps> ocaps = util::ParseAudioCaps(caps);
  if (!ocaps.has_value()) {
		LOG_ERR("Parse audio caps:{} failed!", caps);
		return false;
  }

  // codec
  bool found = false;
  for (auto& item : ocaps->codecs) {
		if (item == ocap->codec) {
			found = true;
			break;
		}
  }
  if (!found) return false;

  // channels
  found = false;
  for (auto& item : ocaps->chnlss) {
		if (item == ocap->chnls) {
			found = true;
			break;
		}
  }
  if (!found) return false;

  // sample bits
  found = false;
  for (auto& item : ocaps->sbitss) {
		if (item == ocap->sbits) {
			found = true;
			break;
		}
  }
  if (!found) return false;

  // sample rate
  found = false;
  for (auto& item : ocaps->srates) {
		if (item == ocap->srate) {
			found = true;
			break;
		}
  }
  if (!found) return false;

  return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MatchVideoCaps(const std::string& cap, const std::string& caps)
{
  std::optional<media::com::VideoCap> ocap = media::util::ParseVideoCap(cap);
  if (!ocap.has_value()) {
		LOG_ERR("Parse video cap:{} failed!", cap);
		return false;
  }

  std::optional<media::com::VideoCaps> ocaps = util::ParseVideoCaps(caps);
  if (!ocaps.has_value()) {
		LOG_ERR("Parse video caps:{} failed!", caps);
		return false;
  }

  // codec
  bool found = false;
  for (auto& item : ocaps->codecs) {
		if (item == ocap->codec) {
			found = true;
			break;
		}
  }
  if (!found) return false;

  // resolution
  found = false;
  for (auto& item : ocaps->ress) {
		if (item == ocap->res) {
			found = true;
			break;
		}
  }
  if (!found) return false;

  // pixel format
  found = false;
  for (auto& item : ocaps->formats) {
		if (item == ocap->format) {
			found = true;
			break;
		}
  }
  if (!found) return false;

  return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<media::com::AudioCap> ToAudioCaps(const stmr::PinCaps& caps)
{
	std::vector<media::com::AudioCap> audio_caps;

	for (const auto& cap : caps) {
		auto cap_opt = media::util::ParseAudioCap(cap);
		if (cap_opt.has_value()) {
			audio_caps.push_back(cap_opt.value());
		}
		else {
			LOG_ERR("Parse audio cap failed, cap:{}", cap);
		}
	}

	return audio_caps;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<media::com::VideoCap> ToVideoCaps(const stmr::PinCaps& caps)
{
	std::vector<media::com::VideoCap> video_caps;

	for (const auto& cap : caps) {
		auto cap_opt = media::util::ParseVideoCap(cap);
		if (cap_opt.has_value()) {
			video_caps.push_back(cap_opt.value());
		}
		else {
			LOG_ERR("Parse video cap failed, cap:{}", cap);
		}
	}

	return video_caps;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::PinCaps ToPinCaps(std::vector<media::com::AudioCap> caps)
{
	stmr::PinCaps pin_caps;

	for (const auto& cap : caps) {
		std::string cap_str = ToAudioCapStr(cap);
		if (!cap_str.empty()) {
			pin_caps.push_back(cap_str);
		}
	}

	return pin_caps;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::PinCaps ToPinCaps(std::vector<media::com::VideoCap> caps)
{
	stmr::PinCaps pin_caps;

	for (const auto& cap : caps) {
		std::string cap_str = ToVideoCapStr(cap);
		if (!cap_str.empty()) {
			pin_caps.push_back(cap_str);
		}
	}

	return pin_caps;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string PinCapsToStr(const stmr::PinCaps& caps)
{
	std::string result;

	result.append("[");

	for (const auto& cap : caps) {
		if (result.length() > 1) {
			result.append(",");
		}
		result.append("\n\t").append(util::Capper(cap));
	}

	result.append("]");
	
	return result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string PinCapsToStr(const std::vector<media::com::VideoCap>& caps)
{
	return PinCapsToStr(ToPinCaps(caps));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::PinCaps MatchAudioPinCapsWithoutCodec(const stmr::PinCaps& src_caps, 
	const stmr::PinCaps& match_caps)
{
	stmr::PinCaps match_result;

	auto match_audio_caps = ToAudioCaps(match_caps);

	for (const auto& src_cap : src_caps) {
		auto audio_cap = ParseAudioCap(src_cap);
		if (!audio_cap.has_value()) {
			LOG_ERR("Parse audio cap failed, cap:{}", src_cap);
			continue;
		}
		
		for (const auto& match_audio_cap : match_audio_caps) {
			if (audio_cap->chnls == match_audio_cap.chnls
				&& audio_cap->sbits == match_audio_cap.sbits
				&& audio_cap->srate == match_audio_cap.srate) {
				match_result.push_back(src_cap);
				break;
			}
		}
	}

	return match_result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::PinCaps MatchAudioPinCapsWithoutCodec(const stmr::PinCaps& src_caps, 
	const std::string& match_cap)
{
	stmr::PinCaps match_caps;
	match_caps.push_back(match_cap);

	return MatchAudioPinCapsWithoutCodec(src_caps, match_caps);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::PinCaps MatchVideoPinCapsWithoutCodec(const stmr::PinCaps& src_caps,
	const stmr::PinCaps& match_caps)
{
	stmr::PinCaps match_result;

	auto match_video_caps = ToVideoCaps(match_caps);

	for (const auto& src_cap : src_caps) {
		auto video_cap = ParseVideoCap(src_cap);
		if (!video_cap.has_value()) {
			LOG_ERR("Parse video cap failed, cap:{}", src_cap);
			continue;
		}

		for (const auto& match_video_cap : match_video_caps) {
			if (video_cap->format == match_video_cap.format
				&& video_cap->res == match_video_cap.res) {
				match_result.push_back(src_cap);
				break;
			}
		}
	}

	return match_result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::PinCaps MatchVideoPinCapsWithoutCodec(const stmr::PinCaps& src_caps,
	const std::string& match_cap)
{
	stmr::PinCaps match_caps;
	match_caps.push_back(match_cap);

	return MatchVideoPinCapsWithoutCodec(src_caps, match_caps);
}

}