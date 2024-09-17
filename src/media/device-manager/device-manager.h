#pragma once

#include "include/if-dev-mgr.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"

#ifdef _WINDOWS
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wmsdkidl.h>
#include <atlconv.h>
#include <atlstr.h>

#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mf.lib")
#pragma comment(lib,"Mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"evr.lib")
#endif

namespace jukey::dev
{

//==============================================================================
// 
//==============================================================================
class DeviceManager 
  : public IDevMgr
  , public base::ProxyUnknown
  , public base::ComObjTracer
{
public:
	DeviceManager(base::IComFactory* factory, const char* owner);

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IDevMgr
	virtual com::ErrCode Init(IDevEventHandler* handler) override;
	virtual std::vector<AudioInputDev> GetAudioInputDevList() override;
	virtual std::vector<VideoInputDev> GetVideoInputDevList() override;
	virtual std::vector<AudioOutputDev> GetAudioOutputDevList() override;

private:
	com::ErrCode EnumAudioInputDevices();
	com::ErrCode EnumVideoInputDevices();
	com::ErrCode EnumAudioOutputDevices();

private:
	struct MediaDevice
	{
		uint32_t dev_id;
		std::string dev_name;
		void* dev_ptr = nullptr; // IMFActivate*
	};

	std::vector<MediaDevice> m_audio_input_dev_list;
	std::vector<MediaDevice> m_video_input_dev_list;
	std::vector<MediaDevice> m_audio_output_dev_list;

	IDevEventHandler* m_handler = nullptr;
};

}