#pragma once

#include "if-media-dev-mgr.h"
#include "if-dev-mgr.h"
#include "com-factory.h"

namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
class MediaDevMgr : public IMediaDevMgr, public dev::IDevEventHandler
{
public:
	static MediaDevMgr& Instance();

	com::ErrCode Init(base::IComFactory* factory);

	// IMediaDevMgr
	virtual void SetEventHandler(sdk::IDevEventHandler* handler) override;
	virtual std::vector<CamDevice> GetCamDevList() override;
	virtual std::vector<MicDevice> GetMicDevList() override;
	virtual std::vector<SpkDevice> GetSpkDevList() override;
	virtual bool GetCamDev(uint32_t dev_id, CamDevice& dev) override;
	virtual bool GetMicDev(uint32_t dev_id, MicDevice& dev) override;
	virtual bool GetSpkDev(uint32_t dev_id, SpkDevice& dev) override;

	// dev::IDevEventHandler
	virtual void OnAddVideoInputDev(const dev::VideoInputDev& dev) override;
	virtual void OnDelVideoInputDev(const dev::VideoInputDev& dev) override;
	virtual void OnAddAudioInputDev(const dev::AudioInputDev& dev) override;
	virtual void OnDelAudioInputDev(const dev::AudioInputDev& dev) override;

private:
	CamDevice ToCamDevice(const dev::VideoInputDev& dev);
	MicDevice ToMicDevice(const dev::AudioInputDev& dev);

private:
	sdk::IDevEventHandler* m_event_handler = nullptr;

	// Media devices
	dev::IDevMgr* m_dev_mgr = nullptr;
};

}