#include <strsafe.h>
#include <atlconv.h>

#include "util-mf.h"
#include "log.h"

#ifndef IF_EQUAL_RETURN
#define IF_EQUAL_RETURN(param, val) if(val == param) return L#val
#endif


namespace jukey::media::util
{

LPCWSTR GetGUIDNameConst(const GUID& guid);

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ToPixelFormat(const GUID sub_type, media::PixelFormat& pf)
{
	if (IsEqualGUID(sub_type, MFVideoFormat_YUY2)) {
		pf = media::PixelFormat::YUY2;
		return true;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_RGB24)) {
		pf = media::PixelFormat::RGB24;
		return true;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_NV12)) {
		pf = media::PixelFormat::NV12;
		return true;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_I420)) {
		pf = media::PixelFormat::I420;
		return true;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_YV12)) {
		LOG_WRN("Unsupport media foundation sub type: YV12");
		return false;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_YVYU)) {
		LOG_WRN("Unsupport media foundation sub type: YVYU");
		return false;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_UYVY)) {
		LOG_WRN("Unsupport media foundation sub type: UYVY");
		return false;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_IYUV)) {
		LOG_WRN("Unsupport media foundation sub type: IYUV");
		return false;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_ARGB32)) {
		LOG_WRN("Unsupport media foundation sub type: ARGB32");
		return false;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_RGB24)) {
		LOG_WRN("Unsupport media foundation sub type: RGB24");
		return false;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_RGB555)) {
		LOG_WRN("Unsupport media foundation sub type: RGB555");
		return false;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_RGB565)) {
		LOG_WRN("Unsupport media foundation sub type: RGB565");
		return false;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_RGB8)) {
		LOG_WRN("Unsupport media foundation sub type: RGB8");
		return false;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_AYUV)) {
		LOG_WRN("Unsupport media foundation sub type: AYUV");
		return false;
	}
	else if (IsEqualGUID(sub_type, MFVideoFormat_YVU9)) {
		LOG_WRN("Unsupport media foundation sub type: YVU9");
		return false;
	}
	else {
		//LOG_WRN("Invalid media foundation sub type!");
		return false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ToMfSubType(media::PixelFormat pf, GUID& sub_type)
{
	switch (pf) {
	case media::PixelFormat::YUY2:
		sub_type = MFVideoFormat_I420;
		return true;
	case media::PixelFormat::RGB24:
		sub_type = MFVideoFormat_RGB24;
		return true;
	case media::PixelFormat::NV12:
		sub_type = MFVideoFormat_NV12;
		return true;
	case media::PixelFormat::I420:
		sub_type = MFVideoFormat_I420;
		return true;
	default:
		LOG_WRN("Unexpected pixel format:{}", pf);
		return false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ToStreamerVideoRes(const PROPVARIANT& var, media::VideoRes& res)
{
	UINT32 uHigh = 0, uLow = 0;
	Unpack2UINT32AsUINT64(var.uhVal.QuadPart, &uHigh, &uLow);

	if (uHigh == 1920 && uLow == 1080) {
		res = media::VideoRes::RES_1920x1080;
		return true;
	}
	else if (uHigh == 1280 && uLow == 720) {
		res = media::VideoRes::RES_1280x720;
		return true;
	}
	else if (uHigh == 640 && uLow == 360) {
		res = media::VideoRes::RES_640x360;
		return true;
	}
	else if (uHigh == 640 && uLow == 480) {
		res = media::VideoRes::RES_640x480;
		return true;
	}
	else if (uHigh == 320 && uLow == 240) {
		res = media::VideoRes::RES_320x240;
		return true;
	}
	else if (uHigh == 320 && uLow == 180) {
		res = media::VideoRes::RES_320x180;
		return true;
	}
	else {
		//LOG_WRN("Unexpected frame size, hight:{}, low:{}", uHigh, uLow);
		return false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ToMfFrameSize(media::VideoRes res, PROPVARIANT& frame_size)
{
	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <class T> void SafeRelease(T** ppT)
{
	if (*ppT) {
		(*ppT)->Release();
		*ppT = NULL;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DBGMSG(PCWSTR format, ...)
{
	va_list args;
	va_start(args, format);

	WCHAR msg[MAX_PATH];

	if (SUCCEEDED(StringCbVPrintf(msg, sizeof(msg), format, args))) {
		OutputDebugString(msg);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LogUINT32AsUINT64(const PROPVARIANT& var)
{
	UINT32 uHigh = 0, uLow = 0;
	Unpack2UINT32AsUINT64(var.uhVal.QuadPart, &uHigh, &uLow);
	//DBGMSG(L"%d x %d", uHigh, uLow);
	//UTIL_INF("{} x {}", uHigh, uLow);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
float OffsetToFloat(const MFOffset& offset)
{
	return offset.value + (static_cast<float>(offset.fract) / 65536.0f);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
HRESULT LogVideoArea(const PROPVARIANT& var)
{
	if (var.caub.cElems < sizeof(MFVideoArea)) {
		//return MF_E_BUFFERTOOSMALL;
		return S_FALSE;
	}

	MFVideoArea* pArea = (MFVideoArea*)var.caub.pElems;

	DBGMSG(L"(%f,%f) (%d,%d)", OffsetToFloat(pArea->OffsetX), OffsetToFloat(pArea->OffsetY),
		pArea->Area.cx, pArea->Area.cy);
	//LOG_INF("({},{}) ({},{})", OffsetToFloat(pArea->OffsetX), OffsetToFloat(pArea->OffsetY),
	//pArea->Area.cx, pArea->Area.cy);
	return S_OK;
}

//------------------------------------------------------------------------------
// Handle certain known special cases.
//------------------------------------------------------------------------------
HRESULT SpecialCaseAttributeValue(GUID guid, const PROPVARIANT& var)
{
	if ((guid == MF_MT_FRAME_RATE) || (guid == MF_MT_FRAME_RATE_RANGE_MAX) ||
		(guid == MF_MT_FRAME_RATE_RANGE_MIN) || (guid == MF_MT_FRAME_SIZE) ||
		(guid == MF_MT_PIXEL_ASPECT_RATIO)) {
		// Attributes that contain two packed 32-bit values.
		LogUINT32AsUINT64(var);
	}
	else if ((guid == MF_MT_GEOMETRIC_APERTURE) ||
		(guid == MF_MT_MINIMUM_DISPLAY_APERTURE) ||
		(guid == MF_MT_PAN_SCAN_APERTURE)) {
		// Attributes that an MFVideoArea structure.
		return LogVideoArea(var);
	}
	else {
		return S_FALSE;
	}
	return S_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
HRESULT GetGUIDName(const GUID& guid, WCHAR** ppwsz)
{
	HRESULT hr = S_OK;
	WCHAR* pName = NULL;

	LPCWSTR pcwsz = GetGUIDNameConst(guid);
	if (pcwsz) {
		size_t cchLength = 0;

		hr = StringCchLength(pcwsz, STRSAFE_MAX_CCH, &cchLength);
		if (FAILED(hr)) {
			goto done;
		}

		pName = (WCHAR*)CoTaskMemAlloc((cchLength + 1) * sizeof(WCHAR));
		if (pName == NULL) {
			hr = E_OUTOFMEMORY;
			goto done;
		}

		hr = StringCchCopy(pName, cchLength + 1, pcwsz);
		if (FAILED(hr)) {
			goto done;
		}
	}
	else {
		hr = StringFromCLSID(guid, &pName);
	}

done:
	if (FAILED(hr)) {
		*ppwsz = NULL;
		CoTaskMemFree(pName);
	}
	else {
		*ppwsz = pName;
	}
	return hr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
HRESULT LogAttributeValueByIndex(IMFAttributes* pAttr, DWORD index)
{
	WCHAR* pGuidName = NULL;
	WCHAR* pGuidValName = NULL;

	USES_CONVERSION;

	GUID guid = { 0 };

	PROPVARIANT var;
	PropVariantInit(&var);

	HRESULT hr = pAttr->GetItemByIndex(index, &guid, &var);
	if (FAILED(hr)) {
		goto done;
	}

	hr = GetGUIDName(guid, &pGuidName);
	if (FAILED(hr)) {
		goto done;
	}

	hr = SpecialCaseAttributeValue(guid, var);
	if (FAILED(hr)) {
		goto done;
	}

	if (hr == S_FALSE) {
		switch (var.vt) {
		case VT_UI4:
			//LOG_INF("{}", var.ulVal);
			break;

		case VT_UI8:
			//LOG_INF("{}:{}", var.uhVal.HighPart, var.uhVal.LowPart);
			break;

		case VT_R8:
			//LOG_INF("{}", var.dblVal);
			break;

		case VT_CLSID:
			//hr = GetGUIDName(*var.puuid, &pGuidValName);
			//if (SUCCEEDED(hr)) {
			//	LOG_INF("{}", W2A(pGuidValName));
			//}
			break;

		case VT_LPWSTR:
			//LOG_INF("{}", W2A(var.pwszVal));
			break;

		case VT_VECTOR | VT_UI1:
			//LOG_INF("<<byte array>>");
			break;

		case VT_UNKNOWN:
			//LOG_INF("IUnknown");
			break;

		default:
			//LOG_INF("Unexpected attribute type (vt = %d)", var.vt);
			break;
		}
	}

done:
	CoTaskMemFree(pGuidName);
	CoTaskMemFree(pGuidValName);
	PropVariantClear(&var);
	return hr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::AudioChnls ToAudioChannels(const PROPVARIANT& var)
{
	UINT32 uHigh = 0, uLow = 0;
	Unpack2UINT32AsUINT64(var.uhVal.QuadPart, &uHigh, &uLow);

	LOG_INF("ChannelNumber:{}|{}", uHigh, uLow);

	switch (uLow) {
	case 1:
		return media::AudioChnls::MONO;
	case 2:
		return media::AudioChnls::STEREO;
	default:
		LOG_WRN("unsupported channel number:{}", uLow);
		return media::AudioChnls::STEREO;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::AudioSRate ToSampleRate(const PROPVARIANT& var)
{
	UINT32 uHigh = 0, uLow = 0;
	Unpack2UINT32AsUINT64(var.uhVal.QuadPart, &uHigh, &uLow);

	LOG_INF("SampleRate:{}|{}", uHigh, uLow);

	switch (uLow) {
	case 48000:
		return media::AudioSRate::SR_48K;
	case 44100:
		LOG_WRN("44100 to 16000");
		return media::AudioSRate::SR_16K;
	case 16000:
		return media::AudioSRate::SR_16K;
	default:
		LOG_WRN("Unsupported sample rate:{}", uLow);
		return media::AudioSRate::SR_16K;
	}
}

//------------------------------------------------------------------------------
// FIXME:
//------------------------------------------------------------------------------
media::AudioSBits ToSBits(const PROPVARIANT& var)
{
	UINT32 uHigh = 0, uLow = 0;
	Unpack2UINT32AsUINT64(var.uhVal.QuadPart, &uHigh, &uLow);

	LOG_INF("SampleBits:{}|{}", uHigh, uLow);

	switch (uLow) {
	case 16:
		return media::AudioSBits::S16;
	case 32:
		//return media::AudioSBits::S32;
		return media::AudioSBits::S16; // TODO: not support
	default:
		LOG_WRN("Unsupported sample bits:{}", uLow);
		return media::AudioSBits::S16;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
HRESULT GetFrameRateRange(IMFMediaType* pType, media::VideoInputDevAttr& attr)
{
	UINT64 maxFrameRate = 0;
	UINT64 minFrameRate = 0;

	// Get the maximum and minimum frame rates.
	HRESULT hr = pType->GetUINT64(MF_MT_FRAME_RATE_RANGE_MAX, &maxFrameRate);
	if (FAILED(hr))
	{
		LOG_ERR("Get max frame rate failed");
		return E_FAIL;
	}
	attr.fps.second = (uint32_t)(maxFrameRate >> 32) / (uint32_t)(maxFrameRate & 0xFFFFFFFF);

	hr = pType->GetUINT64(MF_MT_FRAME_RATE_RANGE_MIN, &minFrameRate);
	if (FAILED(hr))
	{
		LOG_ERR("Get max frame rate failed");
		return E_FAIL;
	}
	attr.fps.first = (uint32_t)(minFrameRate >> 32) / (uint32_t)(minFrameRate & 0xFFFFFFFF);

	return S_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
HRESULT ParseVideoMediaType(IMFMediaType* pType, media::VideoInputDevAttr& attr)
{
	UINT32 count = 0;

	HRESULT hr = pType->GetCount(&count);
	if (FAILED(hr)) {
		return hr;
	}

	if (count == 0) {
		LOG_INF("Empty media type");
		return E_FAIL;
	}

	hr = GetFrameRateRange(pType, attr);
	if (FAILED(hr)) {
		LOG_ERR("Get frame rate range failed");
		return E_FAIL;
	}

	for (UINT32 i = 0; i < count; i++) {
		GUID guid = { 0 };

		PROPVARIANT var;
		PropVariantInit(&var);

		HRESULT hr = pType->GetItemByIndex(i, &guid, &var);
		if (FAILED(hr)) {
			continue;
		}

		if (IsEqualGUID(guid, MF_MT_FRAME_SIZE)) {
			media::VideoRes res;
			if (ToStreamerVideoRes(var, res)) {
				bool found = false;
				for (auto& item : attr.ress) {
					if (item == res) {
						found = true;
						break;
					}
				}
				if (!found) {
					attr.ress.push_back(res);
				}
			}
		}
		else if (IsEqualGUID(guid, MF_MT_SUBTYPE)) {
			media::PixelFormat pf;
			if (ToPixelFormat(*var.puuid, pf)) {
				bool found = false;
				for (auto& item : attr.formats) {
					if (item == pf) {
						found = true;
						break;
					}
				}
				if (!found) {
					attr.formats.push_back(pf);
				}
			}
		}
	}

	return hr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
HRESULT ParseAudioMediaType(IMFMediaType* pType, media::AudioInputDevAttr& attr)
{
	UINT32 count = 0;

	HRESULT hr = pType->GetCount(&count);
	if (FAILED(hr) || count == 0) {
		return hr;
	}

	for (UINT32 i = 0; i < count; i++) {
		GUID guid = { 0 };

		PROPVARIANT var;
		PropVariantInit(&var);

		HRESULT hr = pType->GetItemByIndex(i, &guid, &var);
		if (FAILED(hr)) {
			continue;
		}

		if (IsEqualGUID(guid, MF_MT_AUDIO_BITS_PER_SAMPLE)) {
			attr.sbitss.push_back(ToSBits(var));
		}
		else if (IsEqualGUID(guid, MF_MT_AUDIO_NUM_CHANNELS)) {
			attr.chnlss.push_back(ToAudioChannels(var));
		}
		else if (IsEqualGUID(guid, MF_MT_AUDIO_SAMPLES_PER_SECOND)) {
			attr.srates.push_back(ToSampleRate(var));
		}
	}

	return hr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
HRESULT LogMediaType(IMFMediaType* pType)
{
	UINT32 count = 0;

	HRESULT hr = pType->GetCount(&count);
	if (FAILED(hr)) {
		return hr;
	}

	for (UINT32 i = 0; i < count; i++) {
		LogAttributeValueByIndex(pType, i);
	}

	return S_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
LPCWSTR GetGUIDNameConst(const GUID& guid)
{
	IF_EQUAL_RETURN(guid, MF_MT_MAJOR_TYPE);
	IF_EQUAL_RETURN(guid, MF_MT_MAJOR_TYPE);
	IF_EQUAL_RETURN(guid, MF_MT_SUBTYPE);
	IF_EQUAL_RETURN(guid, MF_MT_ALL_SAMPLES_INDEPENDENT);
	IF_EQUAL_RETURN(guid, MF_MT_FIXED_SIZE_SAMPLES);
	IF_EQUAL_RETURN(guid, MF_MT_COMPRESSED);
	IF_EQUAL_RETURN(guid, MF_MT_SAMPLE_SIZE);
	IF_EQUAL_RETURN(guid, MF_MT_WRAPPED_TYPE);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_NUM_CHANNELS);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_SAMPLES_PER_SECOND);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_AVG_BYTES_PER_SECOND);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_BLOCK_ALIGNMENT);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_BITS_PER_SAMPLE);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_VALID_BITS_PER_SAMPLE);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_SAMPLES_PER_BLOCK);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_CHANNEL_MASK);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_FOLDDOWN_MATRIX);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_PEAKREF);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_PEAKTARGET);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_AVGREF);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_WMADRC_AVGTARGET);
	IF_EQUAL_RETURN(guid, MF_MT_AUDIO_PREFER_WAVEFORMATEX);
	IF_EQUAL_RETURN(guid, MF_MT_AAC_PAYLOAD_TYPE);
	IF_EQUAL_RETURN(guid, MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION);
	IF_EQUAL_RETURN(guid, MF_MT_FRAME_SIZE);
	IF_EQUAL_RETURN(guid, MF_MT_FRAME_RATE);
	IF_EQUAL_RETURN(guid, MF_MT_FRAME_RATE_RANGE_MAX);
	IF_EQUAL_RETURN(guid, MF_MT_FRAME_RATE_RANGE_MIN);
	IF_EQUAL_RETURN(guid, MF_MT_PIXEL_ASPECT_RATIO);
	IF_EQUAL_RETURN(guid, MF_MT_DRM_FLAGS);
	IF_EQUAL_RETURN(guid, MF_MT_PAD_CONTROL_FLAGS);
	IF_EQUAL_RETURN(guid, MF_MT_SOURCE_CONTENT_HINT);
	IF_EQUAL_RETURN(guid, MF_MT_VIDEO_CHROMA_SITING);
	IF_EQUAL_RETURN(guid, MF_MT_INTERLACE_MODE);
	IF_EQUAL_RETURN(guid, MF_MT_TRANSFER_FUNCTION);
	IF_EQUAL_RETURN(guid, MF_MT_VIDEO_PRIMARIES);
	IF_EQUAL_RETURN(guid, MF_MT_CUSTOM_VIDEO_PRIMARIES);
	IF_EQUAL_RETURN(guid, MF_MT_YUV_MATRIX);
	IF_EQUAL_RETURN(guid, MF_MT_VIDEO_LIGHTING);
	IF_EQUAL_RETURN(guid, MF_MT_VIDEO_NOMINAL_RANGE);
	IF_EQUAL_RETURN(guid, MF_MT_GEOMETRIC_APERTURE);
	IF_EQUAL_RETURN(guid, MF_MT_MINIMUM_DISPLAY_APERTURE);
	IF_EQUAL_RETURN(guid, MF_MT_PAN_SCAN_APERTURE);
	IF_EQUAL_RETURN(guid, MF_MT_PAN_SCAN_ENABLED);
	IF_EQUAL_RETURN(guid, MF_MT_AVG_BITRATE);
	IF_EQUAL_RETURN(guid, MF_MT_AVG_BIT_ERROR_RATE);
	IF_EQUAL_RETURN(guid, MF_MT_MAX_KEYFRAME_SPACING);
	IF_EQUAL_RETURN(guid, MF_MT_DEFAULT_STRIDE);
	IF_EQUAL_RETURN(guid, MF_MT_PALETTE);
	IF_EQUAL_RETURN(guid, MF_MT_USER_DATA);
	IF_EQUAL_RETURN(guid, MF_MT_AM_FORMAT_TYPE);
	IF_EQUAL_RETURN(guid, MF_MT_MPEG_START_TIME_CODE);
	IF_EQUAL_RETURN(guid, MF_MT_MPEG2_PROFILE);
	IF_EQUAL_RETURN(guid, MF_MT_MPEG2_LEVEL);
	IF_EQUAL_RETURN(guid, MF_MT_MPEG2_FLAGS);
	IF_EQUAL_RETURN(guid, MF_MT_MPEG_SEQUENCE_HEADER);
	IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_SRC_PACK_0);
	IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_CTRL_PACK_0);
	IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_SRC_PACK_1);
	IF_EQUAL_RETURN(guid, MF_MT_DV_AAUX_CTRL_PACK_1);
	IF_EQUAL_RETURN(guid, MF_MT_DV_VAUX_SRC_PACK);
	IF_EQUAL_RETURN(guid, MF_MT_DV_VAUX_CTRL_PACK);
	IF_EQUAL_RETURN(guid, MF_MT_ARBITRARY_HEADER);
	IF_EQUAL_RETURN(guid, MF_MT_ARBITRARY_FORMAT);
	IF_EQUAL_RETURN(guid, MF_MT_IMAGE_LOSS_TOLERANT);
	IF_EQUAL_RETURN(guid, MF_MT_MPEG4_SAMPLE_DESCRIPTION);
	IF_EQUAL_RETURN(guid, MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY);
	IF_EQUAL_RETURN(guid, MF_MT_ORIGINAL_4CC);
	IF_EQUAL_RETURN(guid, MF_MT_ORIGINAL_WAVE_FORMAT_TAG);

	// Media types

	IF_EQUAL_RETURN(guid, MFMediaType_Audio);
	IF_EQUAL_RETURN(guid, MFMediaType_Video);
	IF_EQUAL_RETURN(guid, MFMediaType_Protected);
	IF_EQUAL_RETURN(guid, MFMediaType_SAMI);
	IF_EQUAL_RETURN(guid, MFMediaType_Script);
	IF_EQUAL_RETURN(guid, MFMediaType_Image);
	IF_EQUAL_RETURN(guid, MFMediaType_HTML);
	IF_EQUAL_RETURN(guid, MFMediaType_Binary);
	IF_EQUAL_RETURN(guid, MFMediaType_FileTransfer);

	IF_EQUAL_RETURN(guid, MFVideoFormat_AI44); //     FCC('AI44')
	IF_EQUAL_RETURN(guid, MFVideoFormat_ARGB32); //   D3DFMT_A8R8G8B8 
	IF_EQUAL_RETURN(guid, MFVideoFormat_AYUV); //     FCC('AYUV')
	IF_EQUAL_RETURN(guid, MFVideoFormat_DV25); //     FCC('dv25')
	IF_EQUAL_RETURN(guid, MFVideoFormat_DV50); //     FCC('dv50')
	IF_EQUAL_RETURN(guid, MFVideoFormat_DVH1); //     FCC('dvh1')
	IF_EQUAL_RETURN(guid, MFVideoFormat_DVSD); //     FCC('dvsd')
	IF_EQUAL_RETURN(guid, MFVideoFormat_DVSL); //     FCC('dvsl')
	IF_EQUAL_RETURN(guid, MFVideoFormat_H264); //     FCC('H264')
	IF_EQUAL_RETURN(guid, MFVideoFormat_I420); //     FCC('I420')
	IF_EQUAL_RETURN(guid, MFVideoFormat_IYUV); //     FCC('IYUV')
	IF_EQUAL_RETURN(guid, MFVideoFormat_M4S2); //     FCC('M4S2')
	IF_EQUAL_RETURN(guid, MFVideoFormat_MJPG);
	IF_EQUAL_RETURN(guid, MFVideoFormat_MP43); //     FCC('MP43')
	IF_EQUAL_RETURN(guid, MFVideoFormat_MP4S); //     FCC('MP4S')
	IF_EQUAL_RETURN(guid, MFVideoFormat_MP4V); //     FCC('MP4V')
	IF_EQUAL_RETURN(guid, MFVideoFormat_MPG1); //     FCC('MPG1')
	IF_EQUAL_RETURN(guid, MFVideoFormat_MSS1); //     FCC('MSS1')
	IF_EQUAL_RETURN(guid, MFVideoFormat_MSS2); //     FCC('MSS2')
	IF_EQUAL_RETURN(guid, MFVideoFormat_NV11); //     FCC('NV11')
	IF_EQUAL_RETURN(guid, MFVideoFormat_NV12); //     FCC('NV12')
	IF_EQUAL_RETURN(guid, MFVideoFormat_P010); //     FCC('P010')
	IF_EQUAL_RETURN(guid, MFVideoFormat_P016); //     FCC('P016')
	IF_EQUAL_RETURN(guid, MFVideoFormat_P210); //     FCC('P210')
	IF_EQUAL_RETURN(guid, MFVideoFormat_P216); //     FCC('P216')
	IF_EQUAL_RETURN(guid, MFVideoFormat_RGB24); //    D3DFMT_R8G8B8 
	IF_EQUAL_RETURN(guid, MFVideoFormat_RGB32); //    D3DFMT_X8R8G8B8 
	IF_EQUAL_RETURN(guid, MFVideoFormat_RGB555); //   D3DFMT_X1R5G5B5 
	IF_EQUAL_RETURN(guid, MFVideoFormat_RGB565); //   D3DFMT_R5G6B5 
	IF_EQUAL_RETURN(guid, MFVideoFormat_RGB8);
	IF_EQUAL_RETURN(guid, MFVideoFormat_UYVY); //     FCC('UYVY')
	IF_EQUAL_RETURN(guid, MFVideoFormat_v210); //     FCC('v210')
	IF_EQUAL_RETURN(guid, MFVideoFormat_v410); //     FCC('v410')
	IF_EQUAL_RETURN(guid, MFVideoFormat_WMV1); //     FCC('WMV1')
	IF_EQUAL_RETURN(guid, MFVideoFormat_WMV2); //     FCC('WMV2')
	IF_EQUAL_RETURN(guid, MFVideoFormat_WMV3); //     FCC('WMV3')
	IF_EQUAL_RETURN(guid, MFVideoFormat_WVC1); //     FCC('WVC1')
	IF_EQUAL_RETURN(guid, MFVideoFormat_Y210); //     FCC('Y210')
	IF_EQUAL_RETURN(guid, MFVideoFormat_Y216); //     FCC('Y216')
	IF_EQUAL_RETURN(guid, MFVideoFormat_Y410); //     FCC('Y410')
	IF_EQUAL_RETURN(guid, MFVideoFormat_Y416); //     FCC('Y416')
	IF_EQUAL_RETURN(guid, MFVideoFormat_Y41P);
	IF_EQUAL_RETURN(guid, MFVideoFormat_Y41T);
	IF_EQUAL_RETURN(guid, MFVideoFormat_YUY2); //     FCC('YUY2')
	IF_EQUAL_RETURN(guid, MFVideoFormat_YV12); //     FCC('YV12')
	IF_EQUAL_RETURN(guid, MFVideoFormat_YVYU);

	IF_EQUAL_RETURN(guid, MFAudioFormat_PCM); //              WAVE_FORMAT_PCM 
	IF_EQUAL_RETURN(guid, MFAudioFormat_Float); //            WAVE_FORMAT_IEEE_FLOAT 
	IF_EQUAL_RETURN(guid, MFAudioFormat_DTS); //              WAVE_FORMAT_DTS 
	IF_EQUAL_RETURN(guid, MFAudioFormat_Dolby_AC3_SPDIF); //  WAVE_FORMAT_DOLBY_AC3_SPDIF 
	IF_EQUAL_RETURN(guid, MFAudioFormat_DRM); //              WAVE_FORMAT_DRM 
	IF_EQUAL_RETURN(guid, MFAudioFormat_WMAudioV8); //        WAVE_FORMAT_WMAUDIO2 
	IF_EQUAL_RETURN(guid, MFAudioFormat_WMAudioV9); //        WAVE_FORMAT_WMAUDIO3 
	IF_EQUAL_RETURN(guid, MFAudioFormat_WMAudio_Lossless); // WAVE_FORMAT_WMAUDIO_LOSSLESS 
	IF_EQUAL_RETURN(guid, MFAudioFormat_WMASPDIF); //         WAVE_FORMAT_WMASPDIF 
	IF_EQUAL_RETURN(guid, MFAudioFormat_MSP1); //             WAVE_FORMAT_WMAVOICE9 
	IF_EQUAL_RETURN(guid, MFAudioFormat_MP3); //              WAVE_FORMAT_MPEGLAYER3 
	IF_EQUAL_RETURN(guid, MFAudioFormat_MPEG); //             WAVE_FORMAT_MPEG 
	IF_EQUAL_RETURN(guid, MFAudioFormat_AAC); //              WAVE_FORMAT_MPEG_HEAAC 
	IF_EQUAL_RETURN(guid, MFAudioFormat_ADTS); //             WAVE_FORMAT_MPEG_ADTS_AAC 

	return NULL;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void EnumVideoInputDevAttr(IMFMediaSource* pSource,
	media::VideoInputDevAttr& attr)
{
	IMFPresentationDescriptor* pPD = NULL;
	IMFStreamDescriptor* pSD = NULL;
	IMFMediaTypeHandler* pHandler = NULL;
	IMFMediaType* pType = NULL;
	DWORD cTypes = 0;

	HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
	if (FAILED(hr)) goto done;

	BOOL fSelected;
	hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
	if (FAILED(hr)) goto done;

	hr = pSD->GetMediaTypeHandler(&pHandler);
	if (FAILED(hr)) goto done;

	hr = pHandler->GetMediaTypeCount(&cTypes);
	if (FAILED(hr)) goto done;

	for (DWORD i = 0; i < cTypes; i++) {
		hr = pHandler->GetMediaTypeByIndex(i, &pType);
		if (FAILED(hr)) goto done;

		if (FAILED(ParseVideoMediaType(pType, attr))) {
			continue;
		}

		LogMediaType(pType);
		SafeRelease(&pType);
	}

done:
	SafeRelease(&pPD);
	SafeRelease(&pSD);
	SafeRelease(&pHandler);
	SafeRelease(&pType);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void EnumAudioInputDevAttr(IMFMediaSource* pSource,
	media::AudioInputDevAttr& attr)
{
	IMFPresentationDescriptor* pPD = NULL;
	IMFStreamDescriptor* pSD = NULL;
	IMFMediaTypeHandler* pHandler = NULL;
	IMFMediaType* pType = NULL;
	DWORD cTypes = 0;

	HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
	if (FAILED(hr)) goto done;

	BOOL fSelected;
	hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
	if (FAILED(hr)) goto done;

	hr = pSD->GetMediaTypeHandler(&pHandler);
	if (FAILED(hr)) goto done;

	hr = pHandler->GetMediaTypeCount(&cTypes);
	if (FAILED(hr)) goto done;

	for (DWORD i = 0; i < cTypes; i++) {
		hr = pHandler->GetMediaTypeByIndex(i, &pType);
		if (FAILED(hr)) goto done;

		if (FAILED(ParseAudioMediaType(pType, attr))) {
			continue;
		}

		LogMediaType(pType);
		SafeRelease(&pType);
	}

done:
	SafeRelease(&pPD);
	SafeRelease(&pSD);
	SafeRelease(&pHandler);
	SafeRelease(&pType);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
HRESULT SetDeviceFormat(IMFMediaSource* pSource, DWORD dwFormatIndex)
{
	IMFPresentationDescriptor* pPD = NULL;
	IMFStreamDescriptor* pSD = NULL;
	IMFMediaTypeHandler* pHandler = NULL;
	IMFMediaType* pType = NULL;

	HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
	if (FAILED(hr)) {
		goto done;
	}

	BOOL fSelected;
	hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
	if (FAILED(hr)) {
		goto done;
	}

	hr = pSD->GetMediaTypeHandler(&pHandler);
	if (FAILED(hr)) {
		goto done;
	}

	hr = pHandler->GetMediaTypeByIndex(dwFormatIndex, &pType);
	if (FAILED(hr)) {
		goto done;
	}

	hr = pHandler->SetCurrentMediaType(pType);

done:
	SafeRelease(&pPD);
	SafeRelease(&pSD);
	SafeRelease(&pHandler);
	SafeRelease(&pType);
	return hr;
}

}