#pragma once

#include <vector>

#include "component.h"
#include "common-enum.h"
#include "common-struct.h"

#include "public/media-struct.h"

namespace jukey::dev
{

// Component ID and interface ID
#define CID_DEV_MGR "cid-dev-mgr"
#define IID_DEV_MGR "iid-dev-mgr"


//==============================================================================
// 
//==============================================================================
struct VideoInputDev
{
	uint32_t dev_id = 0;
	std::string dev_name;
	std::string dev_desc;

	media::VideoInputDevAttr attr;
};

//==============================================================================
// 
//==============================================================================
struct AudioInputDev
{
	uint32_t dev_id = 0;
	std::string dev_name;
	std::string dev_desc;

	media::AudioInputDevAttr attr;
};

//==============================================================================
// 
//==============================================================================
struct AudioOutputDev
{
	uint32_t dev_id = 0;
	std::string dev_name;
	std::string dev_desc;
};

//==============================================================================
// Hot plugging events handler
//==============================================================================
class IDevEventHandler
{
public:
	virtual void OnAddVideoInputDev(const VideoInputDev& dev) = 0;

	virtual void OnDelVideoInputDev(const VideoInputDev& dev) = 0;

	virtual void OnAddAudioInputDev(const AudioInputDev& dev) = 0;

	virtual void OnDelAudioInputDev(const AudioInputDev& dev) = 0;
};

//==============================================================================
// Media device management
//==============================================================================
class IDevMgr : public base::IUnknown
{
public:
	/**
	 * @brief Initialize 
	 */
	virtual com::ErrCode Init(IDevEventHandler* handler) = 0;

	/**
	 * @brief Get audio input devices
	 */
	virtual std::vector<AudioInputDev> GetAudioInputDevList() = 0;

	/**
	 * @brief Get video input devices
	 */
	virtual std::vector<VideoInputDev> GetVideoInputDevList() = 0;


	virtual std::vector<AudioOutputDev> GetAudioOutputDevList() = 0;
};

}