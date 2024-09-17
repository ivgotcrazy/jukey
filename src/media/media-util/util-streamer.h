#pragma once

#include <string>
#include <vector>
#include <optional>

#include "common-define.h"
#include "common-error.h"
#include "if-pin.h"
#include "public/media-enum.h"
#include "common/media-common-struct.h"


namespace jukey::media::util
{

// Parse width from resulotion
uint32_t GetWidth(media::VideoRes resolution);

// Parse height from resulotion
uint32_t GetHeight(media::VideoRes resolution);

// Convert width and height to resulotion
media::VideoRes ToVideoRes(uint32_t width, uint32_t height);

// Image line bytes
uint32_t GetLineBytes(media::VideoRes res, media::PixelFormat format);

// Clone pin data
stmr::PinDataSP ClonePinData(const stmr::PinData& data);


// Enumerate sample rate to number
uint32_t GetSRateNum(media::AudioSRate srate);

// Enumerate channel count to number
uint32_t GetChnlsNum(media::AudioChnls chnls);

// Enumerate bits per sample to number
uint32_t GetSBitsNum(media::AudioSBits sbits);


// Number sample rate to enumerate
media::AudioSRate ToAudioSRate(uint32_t srate);

// Number channel count to enumerate
media::AudioChnls ToAudioChnls(uint32_t chnls);

// Number bits per sample to enumerate
media::AudioSBits ToAudioSBits(uint32_t sbits);


// Protocol value of sample rate to enumerate
media::AudioSRate ToAudioSRateProt(uint8_t srate);

// Protocol value of channel count to enumerate
media::AudioChnls ToAudioChnlsProt(uint8_t chnls);


// Enumerate sample rate to protocol value
uint8_t ToProtAudioSRate(media::AudioSRate srate);

// Enumerate channel count to protocol value
uint8_t ToProtAudioChnls(media::AudioChnls chnls);


// Planar or packed
bool IsPlanar(media::AudioSBits sbits);

// Check validation
bool IsInvalidVideoCap(media::com::VideoCap vcap);

// Check validation
bool InInvalidAudioCap(media::com::AudioCap acap);

// Serialize video capability to json string
std::string ToVideoCapStr(const media::com::VideoCap& cap);

// Serialize video capability to json string by replacing enum with string
std::string ToVideoCapRealStr(const media::com::VideoCap& cap);

// Enum to string
std::string ToVideoCapsRealStr(const media::com::VideoCaps& caps);

// Serialize video capabilities to json string
std::string ToVideoCapsStr(const media::com::VideoCaps& caps);

// Serialize video capability to capabilities json string
std::string ToVideoCapsStr(const media::com::VideoCap& cap);

// Parse video capability from json string
std::optional<media::com::VideoCap> ParseVideoCap(CSTREF cap_str);

// Parse video capabilities from json string
std::optional<media::com::VideoCaps> ParseVideoCaps(CSTREF caps_str);

// Serialize audio capability to json string
std::string ToAudioCapStr(const media::com::AudioCap& cap);

// Serialize audio capability to json string by replacing enum with string
std::string ToAudioCapRealStr(const media::com::AudioCap& cap);

// Enum to string
std::string ToAudioCapsRealStr(const media::com::AudioCaps& caps);

// Serialize audio capabilities to json string
std::string ToAudioCapsStr(const media::com::AudioCaps& caps);

// Serialize audio capability to capabilities json string
std::string ToAudioCapsStr(const media::com::AudioCap& cap);

// Parse audio capability from json string
std::optional<media::com::AudioCap> ParseAudioCap(CSTREF cap_str);

// Parse audio capabilities from json string
std::optional<media::com::AudioCaps> ParseAudioCaps(CSTREF caps_str);

// Replace enumerator in capability with string
std::string Capper(CSTREF cap);

// Replace enumerator in capabilities with string
std::string Cappers(CSTREF caps);

// Permutation and combination of audio capabilities
jukey::com::ErrCode ParseAvaiAudioCaps(const std::string& caps_str,
  std::vector<std::string>& caps);

// Permutation and combination of video capabilities
jukey::com::ErrCode ParseAvaiVideoCaps(const std::string& caps_str,
  std::vector<std::string>& caps);

// Check if audio capability included in capabilities
bool MatchAudioCaps(const std::string& cap, const std::string& caps);

// Check if video capability included in capabilities
bool MatchVideoCaps(const std::string& cap, const std::string& caps);

// Convert string cap to struct cap
std::vector<media::com::AudioCap> ToAudioCaps(const stmr::PinCaps& caps);

// Convert string cap to struct cap
std::vector<media::com::VideoCap> ToVideoCaps(const stmr::PinCaps& caps);

// Convert struct cap to string cap
stmr::PinCaps ToPinCaps(std::vector<media::com::AudioCap> caps);

// Convert struct cap to string cap
stmr::PinCaps ToPinCaps(std::vector<media::com::VideoCap> caps);

std::string PinCapsToStr(const stmr::PinCaps& caps);

std::string PinCapsToStr(const std::vector<media::com::VideoCap>& caps);

// Compare without codec
stmr::PinCaps MatchAudioPinCapsWithoutCodec(const stmr::PinCaps& src_caps,
	const stmr::PinCaps& match_caps);

// Compare witout codec
stmr::PinCaps MatchAudioPinCapsWithoutCodec(const stmr::PinCaps& src_caps,
	const std::string& match_cap);

// Compare without codec
stmr::PinCaps MatchVideoPinCapsWithoutCodec(const stmr::PinCaps& src_caps,
	const stmr::PinCaps& match_caps);

// Compare without codec
stmr::PinCaps MatchVideoPinCapsWithoutCodec(const stmr::PinCaps& src_caps,
	const std::string& match_cap);

}