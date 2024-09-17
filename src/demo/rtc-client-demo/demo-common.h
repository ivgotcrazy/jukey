#pragma once

#include <string>

namespace jukey::demo
{

struct AppSetting
{
	std::string server_addr;
	std::string curr_mic_id;
	std::string curr_cam_id;
	uint32_t curr_cam_resolution = 0;
	uint32_t curr_cam_pixel_foramt = 0;
	uint32_t curr_mic_channel_count = 0;
	uint32_t curr_mic_sample_rate = 0;
	uint32_t curr_mic_sample_bits = 0;
};

}
