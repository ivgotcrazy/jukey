#include "media-player-impl.h"
#include "log.h"

using namespace jukey::com;

namespace jukey::sdk
{

MediaPlayerImpl* MediaPlayerImpl::s_media_player = nullptr;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IMediaPlayer* MediaPlayerImpl::Instance()
{
	// TODO: thread safe
	if (!s_media_player) {
		s_media_player = new MediaPlayerImpl();
	}
	return s_media_player;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerImpl::Release()
{
	if (s_media_player) {
		delete s_media_player;
		s_media_player = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MediaPlayerImpl::MediaPlayerImpl()
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MediaPlayerImpl::CheckParam(const MediaPlayerParam& param)
{
	if (param.wnd == nullptr) {
		LOG_ERR("Invalid wnd");
		return false;
	}

	if (param.handler == nullptr) {
		LOG_ERR("Invalid handler");
		return false;
	}

	if (param.executor == nullptr) {
		LOG_ERR("Invalid executor");
		return false;
	}

	if (param.com_path.empty()) {
		LOG_ERR("Invalid component path");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaPlayerImpl::Init(const MediaPlayerParam& param)
{
	if (!CheckParam(param)) {
		LOG_ERR("Check param failed");
		return ERR_CODE_INVALID_PARAM;
	}
	m_param = param;

	m_factory = GetComFactory();
	if (!m_factory) {
		LOG_ERR("Get component factory failed!");
		return ERR_CODE_FAILED;
	}

	if (!m_factory->Init(m_param.com_path.c_str())) {
		LOG_ERR("Init component factory failed!");
		return ERR_CODE_FAILED;
	}

	m_media_engine = CreateMediaEngine();
	if (!m_media_engine) {
		LOG_ERR("Create media engine failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != m_media_engine->Init(m_factory, this, m_param.executor)) {
		LOG_ERR("Init media engine failed!");
		return ERR_CODE_FAILED;
	}

	m_state = PlayState::INITED;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaPlayerImpl::Open(const std::string& media_file)
{
	LOG_INF("Open media file:{}", media_file);

	if (m_state != PlayState::INITED) {
		LOG_ERR("Invalid state:{}", m_state);
		return ERR_CODE_FAILED;
	}

	m_media_file = media_file;

	if (ERR_CODE_OK != m_media_engine->OpenMediaFile(media_file)) {
		LOG_ERR("Open media file failed");
		return ERR_CODE_FAILED;
	}

	m_state = PlayState::PLAYING; // open to play???

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaPlayerImpl::Start()
{
	

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaPlayerImpl::Pause()
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaPlayerImpl::Stop()
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaPlayerImpl::SeekBegin()
{
	com::MediaSrc msrc;
	msrc.src_type = com::MediaSrcType::FILE;
	msrc.src_id = m_media_file;

	m_media_engine->GetMediaController()->SeekBegin(msrc);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaPlayerImpl::SeekEnd(uint32_t progress)
{
	com::MediaSrc msrc;
	msrc.src_type = com::MediaSrcType::FILE;
	msrc.src_id = m_media_file;

	m_media_engine->GetMediaController()->SeekEnd(msrc, progress);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PlayState MediaPlayerImpl::State()
{
	return m_state;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerImpl::OnAddMediaStream(const com::MediaStream& stream)
{
	if (stream.stream.stream_type == StreamType::AUDIO) {
		if (ERR_CODE_OK != m_media_engine->StartPlayStream(stream)) {
			LOG_ERR("Start play stream failed");
		}
	}
	else if (stream.stream.stream_type == StreamType::VIDEO) {
		if (ERR_CODE_OK != m_media_engine->StartRenderStream(stream, m_param.wnd)) {
			LOG_ERR("Start render stream failed");
		}
	}
	else {
		LOG_ERR("Invalid stream type:{}", stream.stream.stream_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerImpl::OnRemoveMediaStream(const com::MediaStream& stream)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerImpl::OnAudioStreamEnergy(const com::Stream& stream,
	uint32_t energy)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerImpl::OnPlayProgress(const com::MediaSrc& msrc, uint32_t prog)
{
	m_param.handler->OnPlayProgress(msrc, prog);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerImpl::OnRunState(const std::string& desc)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerImpl::OnAudioStreamStats(const com::MediaStream& stream,
	const com::AudioStreamStats& stats)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerImpl::OnVideoStreamStats(const com::MediaStream& stream,
	const com::VideoStreamStats& stats)
{

}

}