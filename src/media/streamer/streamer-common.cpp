#include "streamer-common.h"
#include "util-streamer.h"
#include "util-enum.h"
#include "common/util-string.h"
#include "log.h"

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DumpSrcPin(ISrcPin* pin, std::string result, std::vector<std::string>& results)
{
	std::list<ISinkPin*> sink_pins = pin->SinkPins();

	// End of link, src pin link no sink pins
	if (sink_pins.empty()) {
		results.push_back(result);
	} 
	else { // With sink pins
		for (auto sink_pin : sink_pins) {
			if (sink_pin->Element()) {
				DumpElement(sink_pin->Element(), result, results);
			}
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DumpElement(IElement* element, std::string result, std::vector<std::string>& results)
{
	// New link node
	if (result.empty()) {
		std::string cap = media::util::Capper(element->SrcPins().front()->Cap());
		result.append(element->Name()).append("[").append(cap).append("]");
	}
	else {
		std::string sink_cap = media::util::Capper(element->SinkPins().front()->Cap());
		result.append("\n->").append("[").append(sink_cap).append("]");

		if (!element->SrcPins().empty()) {
			std::string src_cap = media::util::Capper(element->SrcPins().front()->Cap());
			result.append(element->Name()).append("[").append(src_cap).append("]");
		}
		else {
			result.append(element->Name());
		}
	}

	std::vector<ISrcPin*> src_pins = element->SrcPins();

	if (src_pins.empty()) {
		results.push_back(result);
	}
	else {
		for (auto src_pin : src_pins) {
			DumpSrcPin(src_pin, result, results);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LogCallback(void* ptr, int level, const char* fmt, va_list vl)
{
	char buf[1024] = { 0 };
	vsprintf_s(buf, fmt, vl);
	LOG_INF("{}", buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DumpAudioCaps(const media::com::AudioCaps& caps)
{
  for (auto item : caps.codecs) {
    LOG_INF("codec:{}", media::util::AUDIO_CODEC_STR(item));
  }

  for (auto item : caps.chnlss) {
    LOG_INF("channels:{}", media::util::AUDIO_CHNLS_STR(item));
  }

  for (auto item : caps.sbitss) {
    LOG_INF("sample bits:{}", media::util::AUDIO_SBITS_STR(item));
  }

  for (auto item : caps.srates) {
    LOG_INF("sample rate:{}", media::util::AUDIO_SRATE_STR(item));
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DumpVideoCaps(const media::com::VideoCaps& caps)
{
  for (auto item : caps.codecs) {
    LOG_INF("codec:{}", media::util::VIDEO_CODEC_STR(item));
  }

  for (auto item : caps.formats) {
    LOG_INF("format:{}", media::util::VIDEO_FMT_STR(item));
  }

  for (auto item : caps.ress) {
    LOG_INF("resolution:{}", media::util::VIDEO_RES_STR(item));
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DumpPinCaps(const PinCaps& caps)
{
	for (const auto& cap : caps) {
		LOG_INF("{}", media::util::Capper(cap));
	}
}

}