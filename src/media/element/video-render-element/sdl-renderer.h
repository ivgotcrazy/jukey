#pragma once

#include <memory>
#include <queue>

#include "SDL.h"
#include "common-struct.h"
#include "common/media-common-struct.h"
#include "thread/common-thread.h"
#include "if-pin.h"

namespace jukey::stmr
{

class SdlRenderer 
	: public std::enable_shared_from_this<SdlRenderer>
	, public util::CommonThread
{
public:
	SdlRenderer();
	~SdlRenderer();

	bool Init(com::MainThreadExecutor* executor, void* wnd);

	void OnSinkPinNegotiated(const media::com::VideoCap& cap);

	void UpdateAudioTimestamp(uint64_t ts);

	void OnVideoFrame(const PinData& data);

	void FlushData();

	com::ErrCode Start();
	com::ErrCode Pause();
	com::ErrCode Resume();
	com::ErrCode Stop();

	// CommonThread
	virtual void ThreadProc() override;

private:
	com::ErrCode CreateSdlWindow();
	com::ErrCode CreateSdlTexture();
	void ProcessWndResize();
	void ProcessWndRendering();
	void ClearQueueData();
	uint32_t GetTexturePitch();
	uint32_t GetTextureYPitch();
	uint32_t GetTextureUPitch();
	uint32_t GetTextureVPitch();
	void DestroySdlTexture();
	void DestroySdlWindow();

private:
	// SDL
	SDL_Window* m_sdl_window = nullptr;
	SDL_Renderer* m_sdl_renderer = nullptr;
	SDL_Texture* m_sdl_texture = nullptr;

	void* m_render_wnd = nullptr;
	uint32_t m_wnd_width = 1920;
	uint32_t m_wnd_height = 1080;

	bool m_negotiated = false;

	media::com::VideoCap m_sink_pin_cap;

	com::MainThreadExecutor* m_executor = nullptr;

	uint64_t m_audio_pts = 0;

	// Video frame queue
	std::queue<PinDataSP> m_data_que;
	std::mutex m_mutex;
};

}