#pragma once

#include <string>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wmsdkidl.h>

#include "common-struct.h"

#include "public/media-struct.h"
#include "common/media-common-struct.h"

namespace jukey::media::util
{

bool ToPixelFormat(const GUID sub_type, media::PixelFormat& pf);

bool ToMfSubType(media::PixelFormat pf, GUID& sub_type);

bool ToStreamerVideoRes(const PROPVARIANT& frame_size, media::VideoRes& res);

bool ToMfFrameSize(media::VideoRes res, PROPVARIANT& frame_size);

void EnumVideoInputDevAttr(IMFMediaSource* pSource, media::VideoInputDevAttr& attr);
void EnumAudioInputDevAttr(IMFMediaSource* pSource, media::AudioInputDevAttr& attr);

}