#pragma once

namespace jukey::sdk
{

#define G(param) (m_rtc_engine->param)

//==============================================================================
// 
//==============================================================================
enum class RtcEngineState
{
	INVALID,
	INITING,
	INITED,
	LOGINING,
	LOGINED,
	LOGOUTING,
	JOINING,
	JOINED,
	LEAVING
};

//==============================================================================
// 
//==============================================================================
struct RtcEngineData
{
	uint32_t user_id = 0;
	uint32_t group_id = 0;
	uint32_t register_id = 0;
	uint32_t login_id = 0;
};

}