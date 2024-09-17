#pragma once

#include "if-pin.h"
#include "if-element.h"

#include "../common/media-common-struct.h"

namespace jukey::stmr
{

void DumpSrcPin(ISrcPin* pin, std::string result, std::vector<std::string>& results);

void DumpElement(IElement* element, std::string result, std::vector<std::string>& results);

void LogCallback(void* ptr, int level, const char* fmt, va_list vl);

void DumpAudioCaps(const media::com::AudioCaps& caps);

void DumpVideoCaps(const media::com::VideoCaps& caps);

void DumpPinCaps(const PinCaps& caps);

}
