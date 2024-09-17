#include "media-engine-impl.h"
#include "com-factory.h"
#include "common-struct.h"
#include "common-define.h"
#include "common-message.h"
#include "common-enum.h"
#include "engine-common.h"
#include "log.h"
#include "pipeline-msg.h"
#include "media-dev-mgr.h"
#include "engine-util.h"
#include "common/util-property.h"
#include "media-controller.h"

extern "C"
{
#include "SDL.h"
}

using namespace jukey::util;
using namespace jukey::com;


namespace jukey::sdk
{

MediaEngineImpl* MediaEngineImpl::s_engine = nullptr;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MediaEngineImpl::MediaEngineImpl() : CommonThread("MediaEngine", false)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MediaEngineImpl* MediaEngineImpl::Instance()
{
	// TODO: thread safe
	if (!s_engine) {
		s_engine = new MediaEngineImpl();
	}

	return s_engine;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaEngineImpl::Release()
{
	if (s_engine) {
		delete s_engine;
		s_engine = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MediaEngineImpl::~MediaEngineImpl()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MediaEngineImpl::CheckInitParam(const std::string& com_path,
	IMediaEngineHandler* handler, com::MainThreadExecutor* executor)
{
	if (com_path.empty()) {
		LOG_ERR("Empty component path!");
		return false;
	}
	m_com_path = com_path;

	if (!handler) {
		LOG_ERR("Invalid handler!");
		return false;
	}
	m_handler = handler;

	if (!executor) {
		LOG_ERR("Invalid executor!");
		return false;
	}
	m_executor = executor;

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MediaEngineImpl::CheckInitParam(base::IComFactory* factory,
	IMediaEngineHandler* handler, com::MainThreadExecutor* executor)
{
	if (!factory) {
		LOG_ERR("Empty component factory!");
		return false;
	}
	m_factory = factory;

	if (!handler) {
		LOG_ERR("Invalid handler!");
		return false;
	}
	m_handler = handler;

	if (!executor) {
		LOG_ERR("Invalid executor!");
		return false;
	}
	m_executor = executor;

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MediaEngineImpl::DoInit()
{
	//m_msrc_mgr.reset(new MediaSrcHolderMgr(m_factory, m_handler, this));

	//m_sp_mgr.reset(new StreamProcessorMgr(m_factory, 
	//	m_msrc_mgr, m_handler, m_executor));

	//m_controller.reset(new MediaController(m_msrc_mgr));
	m_controller.reset(new MediaController());

	if (ERR_CODE_OK != MediaDevMgr::Instance().Init(m_factory)) {
		LOG_ERR("Init media device manager failed!");
		return false;
	}

	m_sess_mgr = (net::ISessionMgr*)QI(CID_SESSION_MGR, IID_SESSION_MGR,
		"media engine");
	if (!m_sess_mgr) {
		LOG_ERR("Create session manager failed!");
		return false;
	}

	net::SessionMgrParam mgr_param;
	mgr_param.ka_interval = 5;
	mgr_param.thread_count = 2;
	if (ERR_CODE_OK != m_sess_mgr->Init(mgr_param)) {
		LOG_ERR("Initialize session manager failed!");
		return false;
	}

	m_pp_mgr.Init(m_factory, m_sess_mgr, m_executor, this);

	if (0 != SDL_Init(SDL_INIT_EVERYTHING)) {
		LOG_ERR("SDL init failed!");
		return false;
	}

	StartThread();

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::Init(const std::string& com_path, 
	IMediaEngineHandler* handler, com::MainThreadExecutor* executor)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	if (!CheckInitParam(com_path, handler, executor)) {
		LOG_ERR("Invalid initialize parameters!");
		return ERR_CODE_INVALID_PARAM;
	}

	m_factory = GetComFactory();
	if (!m_factory) {
		LOG_ERR("Get component factory failed!");
		return ERR_CODE_FAILED;
	}

	if (!m_factory->Init(com_path.c_str())) {
		LOG_ERR("Init component factory failed!");
		return ERR_CODE_FAILED;
	}

	if (!DoInit()) {
		LOG_ERR("DoInit failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::Init(base::IComFactory* factory, 
	IMediaEngineHandler* handler, com::MainThreadExecutor* executor)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	if (!CheckInitParam(factory, handler, executor)) {
		LOG_ERR("Invalid initialize parameters!");
		return ERR_CODE_INVALID_PARAM;
	}

	// Initialize in main thread once
	if (!DoInit()) {
		LOG_ERR("DoInit failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::OpenCamera(const CamParam& param)
{
	LOG_INF("Open camera, device:{}, resolution:{}, frame rate:{}, pixel format:{}",
		param.dev_id, param.resolution, param.frame_rate, param.pixel_format);

	return m_pp_mgr.OpenCamera(param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::CloseCamera(uint32_t dev_id)
{
	LOG_INF("Close camera:{}", dev_id);

	return m_pp_mgr.CloseCamera(dev_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::OpenMicrophone(const MicParam& param)
{
	LOG_INF("Open microphone, device:{}, channels:{}, rate:{}, bits:{}",
		param.dev_id, param.sample_chnl, param.sample_rate, param.sample_bits);

	return m_pp_mgr.OpenMicrophone(param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::CloseMicrophone(uint32_t dev_id)
{
	LOG_INF("Close microphone:{}", dev_id);

	return m_pp_mgr.CloseMicrophone(dev_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::OpenMediaFile(const std::string& file)
{
	LOG_INF("Open media file:{}", file);

	return m_pp_mgr.OpenMediaFile(file);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::CloseMediaFile(const std::string& file)
{
	LOG_INF("Close media file:{}", file);

	return m_pp_mgr.CloseMediaFile(file);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::OpenNetStream(const MediaStream& stream,
	const std::string& addr)
{
	LOG_INF("Open net stream, app:{}, user:{}, media:{}|{}, stream:{}|{}, addr:{}", 
		stream.src.app_id,
		stream.src.user_id,
		stream.src.src_type,
		stream.src.src_id,
		stream.stream.stream_type,
		stream.stream.stream_id,
		addr);

	return m_pp_mgr.OpenNetStream(stream, addr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::CloseNetStream(const MediaStream& stream)
{
	LOG_INF("Close net stream:{}", stream.stream.stream_id);

	return m_pp_mgr.CloseNetStream(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::StartPlayStream(const com::MediaStream& stream)
{
	LOG_INF("Start play stream:{}", stream.stream.stream_id);

	return m_pp_mgr.StartPlayStream(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::StopPlayStream(const com::MediaStream& stream)
{
	LOG_INF("Stop play stream:{}", stream.stream.stream_id);

	return m_pp_mgr.StopPlayStream(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode
MediaEngineImpl::StartRenderStream(const com::MediaStream& stream, void* wnd)
{
	LOG_INF("Start render stream, media:{}|{}, stream:{}|{}",
		stream.src.src_type,
		stream.src.src_id,
		stream.stream.stream_type,
		stream.stream.stream_id);

	if (!wnd) {
		LOG_ERR("Invalid wnd");
		return ERR_CODE_INVALID_PARAM;
	}

	return m_pp_mgr.StartRenderStream(stream, wnd);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode
MediaEngineImpl::StopRenderStream(const com::MediaStream& stream, void* wnd)
{
	LOG_INF("Stop render stream, media:{}|{}, stream:{}|{}",
		stream.stream.stream_id);

	if (!wnd) {
		LOG_ERR("Invalid wnd");
		return ERR_CODE_INVALID_PARAM;
	}

	return m_pp_mgr.StopRenderStream(stream, wnd);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::StartSendStream(const com::MediaStream& stream,
	const std::string& addr)
{
	LOG_INF("Start send stream:{}", stream.stream.stream_id);

	return m_pp_mgr.StartSendStream(stream, addr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::StopSendStream(const com::MediaStream& stream)
{
	LOG_INF("Stop send stream:{}", stream.stream.stream_id);

	return m_pp_mgr.StopSendStream(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IMediaDevMgr* MediaEngineImpl::GetMediaDevMgr()
{
	return &MediaDevMgr::Instance();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IMediaRecorder* MediaEngineImpl::CreateMediaRecorder()
{
	return nullptr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IMediaControllerSP MediaEngineImpl::GetMediaController()
{
	return m_controller;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
net::ISessionMgr* MediaEngineImpl::GetSessionMgr()
{
	return m_sess_mgr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaEngineImpl::OnPipelineMsg(const com::CommonMsg& msg)
{
	PostMsg(msg); // post to work thread
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaEngineImpl::OnAddMediaStreamMsg(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(MediaStream);

	LOG_INF("Received add media stream message, media:{}|{}, stream:{}|{}",
		data->src.src_type, data->src.src_id,
		data->stream.stream_type, data->stream.stream_id);

	m_handler->OnAddMediaStream(*data);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaEngineImpl::OnRemoveMediaStreamMsg(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(MediaStream);

	LOG_INF("Received remove media stream message, media:{}|{}, stream:{}|{}",
		data->src.src_type, data->src.src_id,
		data->stream.stream_type, data->stream.stream_id);

	m_handler->OnRemoveMediaStream(*data);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaEngineImpl::OnPlayProgressMsg(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(stmr::PlayProgressData);
	m_handler->OnPlayProgress(data->msrc, data->progress);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaEngineImpl::OnRunStateMsg(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(std::string);
	m_handler->OnRunState(*data);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaEngineImpl::OnAudioStreamStatsMsg(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(stmr::AudioStreamStatsData);
	m_handler->OnAudioStreamStats(data->stream, data->stats);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaEngineImpl::OnVideoStreamStatsMsg(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(stmr::VideoStreamStatsData);
	m_handler->OnVideoStreamStats(data->stream, data->stats);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaEngineImpl::OnThreadMsg(const com::CommonMsg& msg)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	switch (msg.msg_type) {
	case ENGINE_MSG_ADD_MEDIA_STREAM:
		OnAddMediaStreamMsg(msg);
		break;
	case ENGINE_MSG_REMOVE_MEDIA_STREAM:
		OnRemoveMediaStreamMsg(msg);
		break;
	case ENGINE_MSG_PLAY_PROGRESS:
		OnPlayProgressMsg(msg);
		break;
	case ENGINE_MSG_RUN_STATE:
		OnRunStateMsg(msg);
		break;
	case ENGINE_MSG_AUDIO_STREAM_STATS:
		OnAudioStreamStatsMsg(msg);
		break;
	case ENGINE_MSG_VIDEO_STREAM_STATS:
		OnVideoStreamStatsMsg(msg);
		break;
	default:
		LOG_ERR("Unknown message type:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::StartAudioTest(const com::MediaStream& stream)
{
	LOG_INF("Start audio test, stream:{}", stream.stream.stream_id);

	return m_pp_mgr.StartAudioTest(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::StopAudioTest(const com::MediaStream& stream)
{
	LOG_INF("Stop audio test, stream:{}", stream.stream.stream_id);

	return m_pp_mgr.StopAudioTest(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::GetStreamInfo(const std::string& stream_id,
	MediaStream& stream)
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MediaEngineImpl::GetMediaSrcStreams(const MediaSrc& msrc,
	std::vector<MediaStream>& streams)
{
	return ERR_CODE_OK;
}

}