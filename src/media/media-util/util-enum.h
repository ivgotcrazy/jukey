#pragma once

#include <string>

#include "if-element.h"
#include "if-pin.h"
#include "public/media-enum.h"

namespace jukey::media::util
{

std::string ELE_SUB_TYPE_STR(stmr::EleSubType stype);
std::string MEDIA_TYPE_STR(media::MediaType mtype);

std::string VIDEO_RES_STR(media::VideoRes res);
std::string VIDEO_FMT_STR(media::PixelFormat format);
std::string VIDEO_CODEC_STR(media::VideoCodec codec);

std::string AUDIO_CODEC_STR(media::AudioCodec codec);
std::string AUDIO_CHNLS_STR(media::AudioChnls chnls);
std::string AUDIO_SRATE_STR(media::AudioSRate srate);
std::string AUDIO_SBITS_STR(media::AudioSBits sbits);

std::string PIN_MSG_STR(stmr::PinMsgType msg_type);

}