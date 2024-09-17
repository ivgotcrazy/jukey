#pragma once

#include <map>

#include "common-struct.h"
#include "common-error.h"

namespace jukey::sdk
{

class RtcEngineImpl;

//==============================================================================
// 
//==============================================================================
class CameraProcessor
{
public:
	CameraProcessor(RtcEngineImpl* engine);

	com::ErrCode OpenCamera(const com::CamParam& param, void* wnd);
	com::ErrCode CloseCamera(uint32_t dev_id);

	void OnAddCamStream(const com::MediaStream& stream);
	void OnDelCamStream(const com::MediaStream& stream);

private:
	struct CameraEntry
	{
		CameraEntry(const com::CamParam& p, void* w) : param(p), wnd(w) {}

		com::CamParam param;

		// TODO: one camera may has more than one renderer
		void* wnd = nullptr;
		com::MediaStream stream;
	};

private:
	RtcEngineImpl* m_rtc_engine = nullptr;

	std::mutex m_mutex;
	std::map<uint32_t, CameraEntry> m_camera_entries;
};

}