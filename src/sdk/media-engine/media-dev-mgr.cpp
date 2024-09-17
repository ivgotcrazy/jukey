#include "media-dev-mgr.h"
#include "engine-common.h"
#include "log.h"
#include "com-factory.h"
#include "common/util-common.h"
#include "common/util-string.h"
#include "util-enum.h"


using namespace jukey::com;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MediaDevMgr& MediaDevMgr::Instance()
{
	static MediaDevMgr mgr;
	return mgr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaDevMgr::Init(base::IComFactory* factory)
{
	LOG_INF("Init MediaDevMgr.");

	m_dev_mgr = (dev::IDevMgr*)factory->QueryInterface(CID_DEV_MGR, IID_DEV_MGR,
		"media deivce manager");
	if (!m_dev_mgr) {
		LOG_ERR("Create device manager failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != m_dev_mgr->Init(this)) {
		LOG_ERR("Init device manager failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaDevMgr::SetEventHandler(sdk::IDevEventHandler* handler)
{
	m_event_handler = handler;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
CamDevice MediaDevMgr::ToCamDevice(const dev::VideoInputDev& dev)
{
	CamDevice cam_dev;
	cam_dev.cam_id = dev.dev_id;
	cam_dev.cam_name = dev.dev_name;
	cam_dev.min_fps = dev.attr.fps.first;
	cam_dev.max_fps = dev.attr.fps.second;

	for (const auto& res : dev.attr.ress) {
		VideoResolution resolution;
		resolution.res_id = (uint32_t)res;
		resolution.res_name = media::util::VIDEO_RES_STR(res);

		cam_dev.res_list.push_back(resolution);
	}

	for (const auto& fmt : dev.attr.formats) {
		VideoFormat format;
		format.format_id = (uint32_t)fmt;
		format.format_name = media::util::VIDEO_FMT_STR(fmt);

		cam_dev.format_list.push_back(format);
	}

	return cam_dev;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MicDevice MediaDevMgr::ToMicDevice(const dev::AudioInputDev& dev)
{
	MicDevice mic_dev;
	mic_dev.mic_id = dev.dev_id;
	mic_dev.mic_name = dev.dev_name;

	for (const auto& item : dev.attr.chnlss) {
		AudioChnls chnls;
		chnls.chnls_id = (uint32_t)item;
		chnls.chnls_name = media::util::AUDIO_CHNLS_STR(item);

		mic_dev.chnls_list.push_back(chnls);
	}

	for (const auto& item : dev.attr.sbitss) {
		AudioSBits sbits;
		sbits.sbits_id = (uint32_t)item;
		sbits.sbits_name = media::util::AUDIO_SBITS_STR(item);

		mic_dev.sbits_list.push_back(sbits);
	}

	for (const auto& item : dev.attr.srates) {
		AudioSRate srate;
		srate.srate_id = (uint32_t)item;
		srate.srate_name = media::util::AUDIO_SRATE_STR(item);

		mic_dev.srate_list.push_back(srate);
	}

	return mic_dev;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<CamDevice> MediaDevMgr::GetCamDevList()
{
	std::vector<dev::VideoInputDev> video_devs = m_dev_mgr->GetVideoInputDevList();

	std::vector<CamDevice> devs;
	for (const auto& video_dev : video_devs) {
		devs.push_back(ToCamDevice(video_dev));
	}

	return devs;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<MicDevice> MediaDevMgr::GetMicDevList()
{
	std::vector<dev::AudioInputDev> audio_devs = m_dev_mgr->GetAudioInputDevList();

	std::vector<MicDevice> devs;
	for (const auto& audio_dev : audio_devs) {
		devs.push_back(ToMicDevice(audio_dev));
	}

	return devs;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<SpkDevice> MediaDevMgr::GetSpkDevList()
{
	std::vector<dev::AudioOutputDev> audio_devs = m_dev_mgr->GetAudioOutputDevList();

	std::vector<SpkDevice> devs;
	for (const auto& audio_dev : audio_devs) {
		SpkDevice dev;
		dev.spk_id = audio_dev.dev_id;
		dev.spk_name = audio_dev.dev_name;

		devs.push_back(dev);
	}

	return devs;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MediaDevMgr::GetCamDev(uint32_t dev_id, CamDevice& dev)
{
	std::vector<CamDevice> cam_devs = GetCamDevList();

	for (const auto& cam_dev : cam_devs) {
		if (cam_dev.cam_id == dev_id) {
			dev = cam_dev;
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MediaDevMgr::GetMicDev(uint32_t dev_id, MicDevice& dev)
{
	std::vector<MicDevice> mic_devs = GetMicDevList();

	for (const auto& mic_dev : mic_devs) {
		if (mic_dev.mic_id == dev_id) {
			dev = mic_dev;
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MediaDevMgr::GetSpkDev(uint32_t dev_id, SpkDevice& dev)
{
	std::vector<SpkDevice> spk_devs = GetSpkDevList();

	for (const auto& spk_dev : spk_devs) {
		if (spk_dev.spk_id == dev_id) {
			dev = spk_dev;
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaDevMgr::OnAddVideoInputDev(const dev::VideoInputDev& dev)
{
	m_event_handler->OnAddCamera(ToCamDevice(dev));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaDevMgr::OnDelVideoInputDev(const dev::VideoInputDev& dev)
{
	m_event_handler->OnDelCamera(ToCamDevice(dev));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaDevMgr::OnAddAudioInputDev(const dev::AudioInputDev& dev)
{
	m_event_handler->OnAddMicrophone(ToMicDevice(dev));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaDevMgr::OnDelAudioInputDev(const dev::AudioInputDev& dev)
{
	m_event_handler->OnDelMicrophone(ToMicDevice(dev));
}

}