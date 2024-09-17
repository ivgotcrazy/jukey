#include "util-enum.h"
#include "log.h"

namespace jukey::media::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string MEDIA_TYPE_STR(media::MediaType mtype)
{
	switch (mtype) {
	case media::MediaType::INVALID:
		return "invalid";
	case media::MediaType::AUDIO:
		return "audio";
	case media::MediaType::VIDEO:
		return "video";
	default:
		return "unknown";
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string VIDEO_RES_STR(media::VideoRes res)
{
	switch (res) {
	case media::VideoRes::INVALID:
		return "invalid";
	case media::VideoRes::ANY:
		return "any";
	case media::VideoRes::RES_320x180:
		return "320x180";
	case media::VideoRes::RES_320x240:
		return "320x240";
	case media::VideoRes::RES_640x360:
		return "640x360";
	case media::VideoRes::RES_640x480:
		return "640x480";
	case media::VideoRes::RES_1280x720:
		return "1280x720";
	case media::VideoRes::RES_1920x1080:
		return "1920x1080";
	default:
		return "unknown";
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string VIDEO_FMT_STR(media::PixelFormat format)
{
	switch (format) {
	case media::PixelFormat::INVALID:
		return "invalid";
	case media::PixelFormat::ANY:
		return "any";
	case media::PixelFormat::I420:
		return "I420";
	case media::PixelFormat::YUY2:
		return "YUY2";
	case media::PixelFormat::NV12:
		return "NV12";
	case media::PixelFormat::YV12:
		return "YV12";
	case media::PixelFormat::RGB24:
		return "RGB24";
	default:
		return "unknown";
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string ELE_SUB_TYPE_STR(stmr::EleSubType stype)
{
	switch (stype) {
	case stmr::EleSubType::INVALID:
		return "invalid";
	case stmr::EleSubType::DEMUXER:
		return "demuxer";
	case stmr::EleSubType::CAPTURER:
		return "capturer";
	case stmr::EleSubType::PROXY:
		return "proxy";
	case stmr::EleSubType::RECEIVER:
		return "stream receiver";
	case stmr::EleSubType::DECODER:
		return "decoder";
	case stmr::EleSubType::ENCODER:
		return "encoder";
	case stmr::EleSubType::CONVERTER:
		return "converter";
	case stmr::EleSubType::MIXER:
		return "mixer";
	case stmr::EleSubType::MUXER:
		return "muxer";
	case stmr::EleSubType::PLAYER:
		return "player";
	case stmr::EleSubType::SENDER:
		return "stream sender";
	case stmr::EleSubType::TESTER:
		return "tester";
	default:
		return "unknown";
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string AUDIO_CODEC_STR(media::AudioCodec codec)
{
	switch (codec) {
	case media::AudioCodec::INVALID:
		return "invalid";
	case media::AudioCodec::ANY:
		return "any";
	case media::AudioCodec::PCM:
		return "pcm";
	case media::AudioCodec::OPUS:
		return "opus";
	case media::AudioCodec::AAC:
		return "aac";
	default:
		return "unknown";
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string AUDIO_CHNLS_STR(media::AudioChnls chnls)
{
	switch (chnls) {
	case media::AudioChnls::INVALID:
		return "invalid";
	case media::AudioChnls::MONO:
		return "mono";
	case media::AudioChnls::STEREO:
		return "stereo";
	case media::AudioChnls::ANY:
		return "any";
	default:
		return "unknown";
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string AUDIO_SRATE_STR(media::AudioSRate srate)
{
	switch (srate) {
	case media::AudioSRate::INVALID:
		return "invalid";
	case media::AudioSRate::ANY:
		return "any";
	case media::AudioSRate::SR_8K:
		return "8000";
	case media::AudioSRate::SR_16K:
		return "16000";
	case media::AudioSRate::SR_48K:
		return "48000";
	default:
		return "unknown";
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string AUDIO_SBITS_STR(media::AudioSBits sbits)
{
	switch (sbits) {
	case media::AudioSBits::INVALID:
		return "invalid";
	case media::AudioSBits::ANY:
		return "any";
	case media::AudioSBits::S16:
		return "S16";
	case media::AudioSBits::S32:
		return "S32";
	case media::AudioSBits::S16P:
		return "S16P";
	case media::AudioSBits::S32P:
		return "S32P";
	case media::AudioSBits::FLTP:
		return "FLTP";
	case media::AudioSBits::DBLP:
		return "DBLP";
	default:
		return "unknown";
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string VIDEO_CODEC_STR(media::VideoCodec codec)
{
	switch (codec) {
	case media::VideoCodec::INVALID:
		return "invalid";
	case media::VideoCodec::ANY:
		return "any";
	case media::VideoCodec::RAW:
		return "raw";
	case media::VideoCodec::H264:
		return "H264";
	case media::VideoCodec::H265:
		return "H265";
	default:
		return "unknown";
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string PIN_MSG_STR(stmr::PinMsgType msg_type)
{
	switch (msg_type) {
	case stmr::PinMsgType::ELE_START:
		return "ELE_START";
	case stmr::PinMsgType::ELE_PAUSE:
		return "ELE_PAUSE";
	case stmr::PinMsgType::ELE_RESUME:
		return "ELE_RESUME";
	case stmr::PinMsgType::ELE_STOP:
		return "ELE_STOP";
	case stmr::PinMsgType::SET_STREAM:
		return "SET_STREAM";
	case stmr::PinMsgType::END_STREAM:
		return "END_STREAM";
	case stmr::PinMsgType::NEGOTIATE:
		return "NEGOTIATE";
	default:
		return "Unknown";
	}
}

}