#include "sdl-renderer.h"
#include "log.h"
#include "util-sdl.h"
#include "util-streamer.h"
#include "common/util-time.h"

using namespace jukey::com;


namespace
{
#include "if-pin.h"

uint64_t CalcTimestamp(const jukey::stmr::PinData& data)
{
	if (data.tbd == 0) {
		return data.pts;
	}
	else {
		return data.pts * data.tbn * 1000 / data.tbd;
	}
}

}

namespace jukey::stmr
{

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
SdlRenderer::SdlRenderer() : CommonThread("SdlRenderer", false)
{

}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
SdlRenderer::~SdlRenderer()
{
	LOG_INF("{}", __FUNCTION__);

	if (m_sdl_texture) {
		SDL_DestroyTexture(m_sdl_texture);
		m_sdl_texture = nullptr;
	}

	if (m_sdl_renderer) {
		SDL_DestroyRenderer(m_sdl_renderer);
		m_sdl_renderer = nullptr;
	}

	if (m_sdl_window) {
		SDL_DestroyWindow(m_sdl_window);
		m_sdl_window = nullptr;
	}
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool SdlRenderer::Init(com::MainThreadExecutor* executor, void* wnd)
{
	m_executor = executor;
	m_render_wnd = wnd;

	m_executor->RunInMainThread([this]()-> void {
		if (ERR_CODE_OK != CreateSdlWindow()) {
			LOG_ERR("Create sdl window failed!");
		}
	});

	return true;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void SdlRenderer::OnSinkPinNegotiated(const media::com::VideoCap& cap)
{
	if (!m_executor) return;

	m_sink_pin_cap = cap;
	m_negotiated = true;

	m_executor->RunInMainThread([this]()->void {
		CreateSdlTexture();
	});
}

//------------------------------------------------------------------------------
// Must run in main thread
//------------------------------------------------------------------------------
ErrCode SdlRenderer::CreateSdlWindow()
{
	if (m_render_wnd) {
		m_sdl_window = SDL_CreateWindowFrom(m_render_wnd);
		if (!m_sdl_window) {
			LOG_ERR("Create SDL window from {} failed, err:{}",
				(intptr_t)m_render_wnd, SDL_GetError());
			return ERR_CODE_FAILED;
		}
		SDL_GetWindowSize(m_sdl_window, (int*)&m_wnd_width, (int*)&m_wnd_height);
		LOG_INF("Window size, width:{}, height:{}", m_wnd_width, m_wnd_height);
	}
	else {
		m_sdl_window = SDL_CreateWindow(
			"VideoRenderer",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			m_wnd_width,
			m_wnd_height,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
		if (!m_sdl_window) {
			LOG_ERR("Create SDL window failed, err:{}", SDL_GetError());
			return ERR_CODE_FAILED;
		}
	}

	m_sdl_renderer = SDL_CreateRenderer(m_sdl_window, -1, 0);
	if (!m_sdl_renderer) {
		LOG_ERR("Create SDL renderer failed!");
		SDL_DestroyWindow(m_sdl_window);
		m_sdl_window = nullptr;
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SdlRenderer::CreateSdlTexture()
{
	if (!m_negotiated) return ERR_CODE_OK;

	if (m_sdl_renderer) {
		m_sdl_texture = SDL_CreateTexture(
			m_sdl_renderer,
			media::util::ToSdlPixelFormat(m_sink_pin_cap.format),
			SDL_TEXTUREACCESS_STREAMING,
			media::util::GetWidth(m_sink_pin_cap.res),
			media::util::GetHeight(m_sink_pin_cap.res));
		if (!m_sdl_texture) {
			LOG_ERR("Create SDL texture failed!");
			return ERR_CODE_FAILED;
		}
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdlRenderer::ThreadProc()
{
	while (!m_stop) {
		auto self = shared_from_this();
		m_executor->RunInMainThread([self]()->void {
			std::lock_guard<std::mutex> lock(self->m_mutex);
			self->ProcessWndResize();
			self->ProcessWndRendering();
			});

		util::Sleep(10000); // 10ms
	}

	LOG_INF("Exit thread");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdlRenderer::OnVideoFrame(const PinData& data)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_data_que.size() > 0
		&& CalcTimestamp(data) <= CalcTimestamp(*m_data_que.back().get())) {
		ClearQueueData();
		LOG_WRN("Clear frames from queue for invalid timestamp");
	}

	m_data_que.push(media::util::ClonePinData(data));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdlRenderer::UpdateAudioTimestamp(uint64_t ts)
{
	m_audio_pts = ts;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdlRenderer::ProcessWndResize()
{
	uint32_t wnd_width, wnd_height;

	SDL_GetWindowSize(m_sdl_window, (int*)&wnd_width, (int*)&wnd_height);

	if (wnd_width == m_wnd_width && wnd_height == m_wnd_height)
		return;

	LOG_INF("old wnd:{}|{}, new wnd:{}|{}", m_wnd_width, m_wnd_height,
		wnd_width, wnd_height);

	m_wnd_width = wnd_width;
	m_wnd_height = wnd_height;

	// FIXME: destroy will not work
	//DestroySdlTexture();
	//DestroySdlWindow();

	CreateSdlWindow();
	CreateSdlTexture();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdlRenderer::ProcessWndRendering()
{
	if (m_data_que.empty())
		return;

	PinDataSP data = m_data_que.front();

	uint64_t video_pts = data->pts;
	if (data->tbd != 0) {
		video_pts = data->pts * data->tbn * 1000 / data->tbd;
	}

	// 未开启同步或者视频时间戳落后于音频时间戳
	if (m_audio_pts == 0 || video_pts <= m_audio_pts) {
		if (data->data_count == 1) { // packed
			SDL_UpdateTexture(m_sdl_texture,
				NULL,
				data->media_data[0].data.get(),
				GetTexturePitch());
		}
		else { // planar
			SDL_UpdateYUVTexture(m_sdl_texture,
				NULL,
				data->media_data[0].data.get(),
				GetTextureYPitch(),
				data->media_data[1].data.get(),
				GetTextureUPitch(),
				data->media_data[2].data.get(),
				GetTextureVPitch());
		}

		SDL_RenderClear(m_sdl_renderer);
		SDL_RenderCopy(m_sdl_renderer, m_sdl_texture, NULL, NULL);
		SDL_RenderPresent(m_sdl_renderer);

		m_data_que.pop();
	}
}


//------------------------------------------------------------------------------
// Pitch: the number of bytes in a row of pixel data, including padding between 
//        lines
//------------------------------------------------------------------------------
uint32_t SdlRenderer::GetTexturePitch()
{
	//uint32_t pixels = media::util::GetWidth(m_sink_pin_cap.res);
	//uint32_t bits_per_pixel = util::GetBitsPerPixel(m_sink_pin_cap.format);
	//return pixels * bits_per_pixel / 8;

	return media::util::GetWidth(m_sink_pin_cap.res);
}

//------------------------------------------------------------------------------
// Is it?
//------------------------------------------------------------------------------
uint32_t SdlRenderer::GetTextureYPitch()
{
	return media::util::GetWidth(m_sink_pin_cap.res);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t SdlRenderer::GetTextureUPitch()
{
	switch (m_sink_pin_cap.format) {
	case media::PixelFormat::NV12: // 4:2:0
		return media::util::GetWidth(m_sink_pin_cap.res) / 2;
	case media::PixelFormat::YV12: // 4:2:0
		return media::util::GetWidth(m_sink_pin_cap.res) / 2;
	case media::PixelFormat::I420: // 4:2:0
		return media::util::GetWidth(m_sink_pin_cap.res) / 2;
	default:
		LOG_ERR("Unexpected pixel format:{}", m_sink_pin_cap.format);
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t SdlRenderer::GetTextureVPitch()
{
	switch (m_sink_pin_cap.format) {
	case media::PixelFormat::NV12: // 4:2:0
		return 0;
	case media::PixelFormat::YV12: // 4:2:0
		return 0;
	case media::PixelFormat::I420: // 4:2:0
		return media::util::GetWidth(m_sink_pin_cap.res) / 2;
	default:
		LOG_ERR("Unexpected pixel format:{}", m_sink_pin_cap.format);
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdlRenderer::ClearQueueData()
{
	std::queue<PinDataSP> empty_que;
	m_data_que.swap(empty_que);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdlRenderer::FlushData()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	ClearQueueData();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdlRenderer::DestroySdlWindow()
{
	if (m_sdl_renderer) {
		SDL_DestroyRenderer(m_sdl_renderer);
		m_sdl_renderer = nullptr;
	}

	if (m_sdl_window) {
		SDL_DestroyWindow(m_sdl_window);
		m_sdl_window = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdlRenderer::DestroySdlTexture()
{
	if (m_sdl_texture) {
		SDL_DestroyTexture(m_sdl_texture);
		m_sdl_texture = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SdlRenderer::Start()
{
	StartThread();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SdlRenderer::Pause()
{
	DestroySdlTexture();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SdlRenderer::Resume()
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SdlRenderer::Stop()
{
	StopThread();

	DestroySdlTexture();

	return ERR_CODE_OK;
}

}