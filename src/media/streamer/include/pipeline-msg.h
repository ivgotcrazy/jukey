#pragma once

#include <string>
#include "common-struct.h"

namespace jukey::stmr
{

//==============================================================================
// Unified define of pipeline message, avoid confliction
// TODO: Element define coupled
//==============================================================================
enum PlMsgType
{
	INVALID               = 8000,

	ADD_ELEMENT_STREAM    = 8001,
	DEL_ELEMENT_STREAM    = 8002,
	SET_RENDER_WND        = 8003,
	REMOVE_RENDERER       = 8004,
	REMOVE_PLAYER         = 8005,
	ADD_SINK_PIN          = 8006,
	REMOVE_SINK_PIN       = 8007,
	ADD_SRC_PIN           = 8008,
	REMOVE_SRC_PIN        = 8009,
	NEGOTIATE             = 8010,
	START_ELEMENT         = 8011,
	STOP_ELEMENT          = 8012,
	PAUSE_ELEMENT         = 8013,
	RESUME_ELEMENT        = 8014,
	AUDIO_ENERGY          = 8015,
	PLAY_PROGRESS         = 8016,
	PL_SEEK_BEGIN         = 8017,
	PL_SEEK_END           = 8018,
	RUN_STATE             = 8019,
	AUIDO_STREAM_STATS    = 8020,
	VIDEO_STREAM_STATS    = 8021,

	BUTT
};

//==============================================================================
// 
//==============================================================================
struct AddElementData
{
	AddElementData() {}
	AddElementData(void* ele) : element(ele) {}

	void* element = nullptr;
};

//==============================================================================
// 
//==============================================================================
struct RemoveElementData
{
	RemoveElementData() {}
	RemoveElementData(void* ele) : element(ele) {}

	void* element = nullptr;
};

//==============================================================================
// 
//==============================================================================
typedef std::function<void(void*)> MainThreadTask;
class IMainThreadExecutor
{
public:
	virtual void PostTask(MainThreadTask task, void* param) = 0;
};

//==============================================================================
// 
//==============================================================================
struct RenderWndData
{
	RenderWndData(void* wnd) : render_wnd(wnd) {}

	void* render_wnd = nullptr;
};

//==============================================================================
// 
//==============================================================================
struct VideoDevParamData
{
	VideoDevParamData(void* dev, uint32_t fr, media::VideoRes vr)
		: device(dev), frame_rate(fr), resolution(vr) {}

	void* device = nullptr;
	uint32_t frame_rate = 0;
	media::VideoRes resolution = media::VideoRes::INVALID;
};
typedef std::shared_ptr<VideoDevParamData> VideoDevParamDataSP;

//==============================================================================
// 
//==============================================================================
struct RemoveRendererData
{
	RemoveRendererData(void* wnd) : render_wnd(wnd) {}

	void* render_wnd;
};
typedef std::shared_ptr<RemoveRendererData> RemoveRendererDataSP;

//==============================================================================
// 
//==============================================================================
struct AudioEnergyData
{
	AudioEnergyData(const std::string& id, uint32_t e)
		: stream_id(id), energy(e) {}

	std::string stream_id;
	uint32_t energy;
};
typedef std::shared_ptr<AudioEnergyData> AudioEnergyDataSP;

//==============================================================================
// 
//==============================================================================
struct PlayProgressData
{
	PlayProgressData(const com::MediaSrc& m, uint32_t p) : msrc(m), progress(p) {}

	com::MediaSrc msrc;
	uint32_t progress = 0;
};
typedef std::shared_ptr<PlayProgressData> PlayProgressDataSP;

//==============================================================================
// 
//==============================================================================
struct SeekEndMsgData
{
	SeekEndMsgData(uint32_t p) : progress(p) {}

	uint32_t progress = 0;
};
typedef std::shared_ptr<SeekEndMsgData> SeekEndMsgDataSP;

//==============================================================================
// 
//==============================================================================
struct AudioStreamStatsData
{
	AudioStreamStatsData() {}
	AudioStreamStatsData(const com::MediaStream stm,
		const com::AudioStreamStats& sts)
		: stream(stm), stats(sts) {}

	com::MediaStream stream;
	com::AudioStreamStats stats;
};
typedef std::shared_ptr<AudioStreamStatsData> AudioStreamStatsDataSP;

//==============================================================================
// 
//==============================================================================
struct VideoStreamStatsData
{
	VideoStreamStatsData() {}
	VideoStreamStatsData(const com::MediaStream stm,
		const com::VideoStreamStats& sts)
		: stream(stm), stats(sts) {}

	com::MediaStream stream;
	com::VideoStreamStats stats;
};
typedef std::shared_ptr<VideoStreamStatsData> VideoStreamStatsDataSP;

}