#pragma once

#include <vector>

#include "common-error.h"
#include "common-struct.h"

namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
struct VideoResolution
{
	uint32_t res_id = 0;
	std::string res_name;
};

//==============================================================================
// 
//==============================================================================
struct VideoFormat
{
	uint32_t format_id = 0;
	std::string format_name;
};

//==============================================================================
// 
//==============================================================================
struct CamDevice
{
	uint32_t cam_id = 0;
	std::string cam_name;
	
	uint32_t max_fps = 0;
	uint32_t min_fps = 0;
	
	std::vector<VideoResolution> res_list;
	std::vector<VideoFormat> format_list;
};

//==============================================================================
// 
//==============================================================================
struct AudioChnls
{
	uint32_t chnls_id = 0;
	std::string chnls_name;
};

//==============================================================================
// 
//==============================================================================
struct AudioSRate
{
	uint32_t srate_id = 0;
	std::string srate_name;
};

//==============================================================================
// 
//==============================================================================
struct AudioSBits
{
	uint32_t sbits_id = 0;
	std::string sbits_name;
};

//==============================================================================
// 
//==============================================================================
struct MicDevice
{
	uint32_t mic_id = 0;
	std::string mic_name;

	std::vector<AudioChnls> chnls_list;
	std::vector<AudioSBits> sbits_list;
	std::vector<AudioSRate> srate_list;
};

//==============================================================================
// 
//==============================================================================
struct SpkDevice
{
	uint32_t spk_id = 0;
	std::string spk_name;
};

//==============================================================================
// 
//==============================================================================
class IDevEventHandler
{
public:
	virtual ~IDevEventHandler() {}

	//
	// Add device callback
	//
	virtual void OnAddCamera(const CamDevice& device) = 0;

	//
	// Remove device callback
	//
	virtual void OnDelCamera(const CamDevice& device) = 0;

	//
	// Add device callback
	//
	virtual void OnAddMicrophone(const MicDevice& device) = 0;

	//
	// Remove device callback
	//
	virtual void OnDelMicrophone(const MicDevice& device) = 0;
	
};

//==============================================================================
// 
//==============================================================================
class IMediaDevMgr
{
public:
	virtual ~IMediaDevMgr() {}

	//
	// Set event handler
	//
	virtual void SetEventHandler(sdk::IDevEventHandler* handler) = 0;

	//
	// Get camera devices
	//
	virtual std::vector<CamDevice> GetCamDevList() = 0;

	//
	// Get microphone devices
	//
	virtual std::vector<MicDevice> GetMicDevList() = 0;

	//
	// Get speaker devices
	//
	virtual std::vector<SpkDevice> GetSpkDevList() = 0;

	//
	// Get camera device
	//
	virtual bool GetCamDev(uint32_t dev_id, CamDevice& dev) = 0;

	//
	// Get microphone device
	//
	virtual bool GetMicDev(uint32_t dev_id, MicDevice& dev) = 0;

	//
	// Get speaker device
	//
	virtual bool GetSpkDev(uint32_t dev_id, SpkDevice& dev) = 0;
};

}