#pragma once

#include <string>

namespace jukey::demo
{

//==============================================================================
// 
//==============================================================================
struct MicSetting
{
	uint32_t curr_mic = 0;

	uint32_t sample_rate = 16000;
	uint32_t sample_chnl = 1;
	uint32_t sample_bits = 16;
};

//==============================================================================
// 
//==============================================================================
struct SpkSetting
{
	uint32_t curr_spk = 0;
};

//==============================================================================
// 
//==============================================================================
struct CamSetting
{
	uint32_t curr_cam = 0;
	uint32_t resolution = 6;   // RES_1280x720
	uint32_t pixel_format = 4; // NV12
	uint32_t frame_rate = 30;
};

//==============================================================================
// 
//==============================================================================
struct DemoSetting
{
	MicSetting mic_setting;
	SpkSetting spk_setting;
	CamSetting cam_setting;
};

//==============================================================================
// 
//==============================================================================
class SettingMgr
{
public:
	static SettingMgr& Instance();

	bool Init(const std::string& db_file);

	DemoSetting& Setting() { return m_demo_setting; }

	void UpdateSetting();

private:
	DemoSetting m_demo_setting;
	std::string m_db_file;
};

}