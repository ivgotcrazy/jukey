#include "sdk-mgr.h"
#include "config-parser.h"
#include "render-mgr.h"
#include "log.h"
#include <sstream>


using namespace jukey::sdk;
using namespace jukey::com;
using namespace std::placeholders;

#define EXECUTE_EVENT	(QEvent::Type)(QEvent::User + 100)

namespace jukey::demo
{

//------------------------------------------------------------------------------
//  
//------------------------------------------------------------------------------
SdkMgr::SdkMgr()
{
	QEvent::registerEventType(EXECUTE_EVENT);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::event(QEvent* event)
{
	if (event->type() == EXECUTE_EVENT) {
		ExecuteEvent* e = (ExecuteEvent*)event;
		e->task();
		return true;
	}
	else {
		LOG_ERR("Unknown event type:{}", event->type());
		return false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::RunInMainThread(std::function<void()> task)
{
	ExecuteEvent* e = new ExecuteEvent(EXECUTE_EVENT);
	e->task = task;

	m_app->postEvent(this, e);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::Init(QCoreApplication* app, ISdkEventHandler* handler)
{
	m_rtc_engine = CreateRtcEngine();
	if (!m_rtc_engine) {
		LOG_ERR("Create rtc engine failed");
		return false;
	}

	auto config = ConfigParser::Instance().Config();

	RtcEngineParam param;
	param.app_id = config.app_id;
	param.client_id = config.client_id;
	param.client_name = config.client_name;
	param.address = config.server_addr;
	param.com_path = QDir::currentPath().toStdString().c_str();
	param.handler = this;
	param.executor = this;

	if (ERR_CODE_OK != m_rtc_engine->Init(param)) {
		LOG_ERR("Init rtc engine failed");
		return false;
	}

	m_dev_mgr = m_rtc_engine->GetMediaDevMgr();

	m_app = app;
	m_event_handler = handler;

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::Login(uint32_t user_id)
{
	LOG_INF("Login, user ID:{}", user_id);

	if (ERR_CODE_OK != m_rtc_engine->Login(user_id)) {
		LOG_ERR("Login failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::Logout()
{
	LOG_INF("Logout");

	if (ERR_CODE_OK != m_rtc_engine->Logout()) {
		LOG_ERR("Logout failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::JoinGroup(uint32_t group_id)
{
	LOG_INF("Join group, group ID:{}", group_id);

	if (ERR_CODE_OK != m_rtc_engine->JoinGroup(group_id)) {
		LOG_ERR("Join group failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::LeaveGroup()
{
	LOG_INF("Leave group");

	if (ERR_CODE_OK != m_rtc_engine->LeaveGroup()) {
		LOG_ERR("Leave group failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<CamDevice> SdkMgr::GetCamDevices()
{
	return m_rtc_engine->GetMediaDevMgr()->GetCamDevList();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<MicDevice> SdkMgr::GetMicDevices()
{
	return m_rtc_engine->GetMediaDevMgr()->GetMicDevList();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<SpkDevice> SdkMgr::GetSpkDevices()
{
	return m_rtc_engine->GetMediaDevMgr()->GetSpkDevList();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::GetCamDevice(uint32_t dev_id, CamDevice& dev)
{
	return m_rtc_engine->GetMediaDevMgr()->GetCamDev(dev_id, dev);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::GetMicDevice(uint32_t dev_id, MicDevice& dev)
{
	return m_rtc_engine->GetMediaDevMgr()->GetMicDev(dev_id, dev);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::GetSpkDevice(uint32_t dev_id, SpkDevice& dev)
{
	return m_rtc_engine->GetMediaDevMgr()->GetSpkDev(dev_id, dev);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::OpenCamera(const CamParam& param)
{
	LOG_INF("Open camera, device:{}, reslution:{}, frame rate:{}, pixel format:{}",
		param.dev_id, param.resolution, param.frame_rate, param.pixel_format);

	void* wnd = RenderMgr::Instance().AddRender(std::to_string(param.dev_id));
	if (wnd == nullptr) {
		LOG_ERR("Get render window failed");
		return false;
	}

	ErrCode result = m_rtc_engine->OpenCamera(param, wnd);
	if (result != ERR_CODE_OK) {
		LOG_ERR("Open camera failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::CloseCamera(uint32_t dev_id)
{
	LOG_INF("Close camera, device:{}", dev_id);

	// Remove renderer
	RenderMgr::Instance().RemoveRender(std::to_string(dev_id));

	if (ERR_CODE_OK != m_rtc_engine->CloseCamera(dev_id)) {
		LOG_ERR("Close camera failed");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::OpenMicrophone(const MicParam& param)
{
	LOG_INF("Open microphone, device:{}, channels:{}, rate:{}, bits:{}",
		param.dev_id, param.sample_chnl, param.sample_rate, param.sample_bits);

	ErrCode result = m_rtc_engine->OpenMic(param);
	if (result != ERR_CODE_OK) {
		LOG_ERR("Open microphone failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::CloseMicrophone(uint32_t dev_id)
{
	LOG_INF("Close microphone, device:{}", dev_id);

	m_rtc_engine->CloseMic(dev_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::OpenMediaFile(const std::string& file)
{
	void* wnd = RenderMgr::Instance().AddRender(file);
	if (wnd == nullptr) {
		LOG_ERR("Get render window failed");
		return false;
	}

	if (ERR_CODE_FAILED != m_rtc_engine->OpenMediaFile(file, wnd)) {
		LOG_ERR("Open media file failed!");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::CloseMediaFile(const std::string& file)
{
	LOG_INF("Close media file:{}", file);

	// Remove renderer
	RenderMgr::Instance().RemoveRender(file);

	if (ERR_CODE_OK != m_rtc_engine->CloseMediaFile(file)) {
		LOG_ERR("Close media file failed");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::StartRecvAudio(const MediaStream& stream)
{
	LOG_INF("Start receiving audio, app:{}, user:{}, stream:{}|{}, media:{}|{}",
		stream.src.app_id,
		stream.src.user_id,
		stream.stream.stream_type,
		stream.stream.stream_id,
		stream.src.src_type,
		stream.src.src_id);

	ErrCode result = m_rtc_engine->StartRecvAudio(stream);
	if (result != ERR_CODE_OK) {
		LOG_ERR("Subscribe audio stream failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::StopRecvAudio(const MediaStream& stream)
{
	LOG_INF("Stop receiving audio, app:{}, user:{}, stream:{}|{}, media:{}|{}",
		stream.src.app_id,
		stream.src.user_id,
		stream.stream.stream_type,
		stream.stream.stream_id,
		stream.src.src_type,
		stream.src.src_id);

	ErrCode result = m_rtc_engine->StopRecvAudio(stream);
	if (result != ERR_CODE_OK) {
		LOG_ERR("Unsubscribe audio stream failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::StartRecvVideo(const MediaStream& stream, void* wnd)
{
	LOG_INF("Start receiving video, app:{}, user:{}, stream:{}|{}, media:{}|{}",
		stream.src.app_id,
		stream.src.user_id,
		stream.stream.stream_type,
		stream.stream.stream_id,
		stream.src.src_type,
		stream.src.src_id);

	ErrCode result = m_rtc_engine->StartRecvVideo(stream, wnd);
	if (result != ERR_CODE_OK) {
		LOG_ERR("Subscribe video stream failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::StopRecvVideo(const MediaStream& stream, void* wnd)
{
	LOG_INF("Stop receiving video, app:{}, user:{}, stream:{}|{}, media:{}|{}",
		stream.src.app_id,
		stream.src.user_id,
		stream.stream.stream_type,
		stream.stream.stream_id,
		stream.src.src_type,
		stream.src.src_id);

	ErrCode result = m_rtc_engine->StopRecvVideo(stream, wnd);
	if (result != ERR_CODE_OK) {
		LOG_ERR("Unsubscribe video stream failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnSubStreamResult(const MediaStream& stream, ErrCode result)
{
	std::cout << __FUNCTION__ << std::endl;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnPubAudioNotify(uint32_t group_id, const MediaStream& stream)
{
	RunInMainThread([this, stream]() -> void {
		if (m_event_handler) {
			m_event_handler->OnPubStream(stream);
		}
	});

	DoStartRecvAudio(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::DoStartRecvAudio(const MediaStream& stream)
{
	if (ERR_CODE_OK != m_rtc_engine->StartRecvAudio(stream)) {
		LOG_ERR("Start recv audio failed");
	}

	m_streams.insert(std::make_pair(stream.stream.stream_id, stream));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnUnpubAudioNotify(uint32_t group_id, const MediaStream& stream)
{
	RunInMainThread([this, stream]() -> void {
		if (m_event_handler) {
			m_event_handler->OnUnpubStream(stream);
		}
	});

	DoStopRecvAudio(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::DoStopRecvAudio(const MediaStream& stream)
{
	if (ERR_CODE_OK != m_rtc_engine->StopRecvAudio(stream)) {
		LOG_ERR("Stop recv audio failed");
	}

	m_streams.erase(stream.stream.stream_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnPubVideoNotify(uint32_t group_id, const MediaStream& stream)
{
	if (m_streams.find(stream.stream.stream_id) != m_streams.end()) {
		LOG_ERR("Repeat publish video stream:{}", stream.stream.stream_id);
		return;
	}

	RunInMainThread([this, stream]() -> void {
		if (m_event_handler) {
			m_event_handler->OnPubStream(stream);
		}
	});

	DoStartRecvVideo(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::DoStartRecvVideo(const MediaStream& stream)
{
	RunInMainThread([this, stream]() -> void {
		void* wnd = RenderMgr::Instance().AddRender(STRM_ID(stream));
		if (wnd == nullptr) {
			LOG_ERR("Get renderer window failed");
		}

		if (ERR_CODE_OK != m_rtc_engine->StartRecvVideo(stream, wnd)) {
			LOG_ERR("Start recv video stream failed");
		}
	});

	m_streams.insert(std::make_pair(stream.stream.stream_id, stream));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnUnpubVideoNotify(uint32_t group_id, const MediaStream& stream)
{
	RunInMainThread([this, stream]() -> void {
		if (m_event_handler) {
			m_event_handler->OnUnpubStream(stream);
		}
	});

	DoStopRecvVideo(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::DoStopRecvVideo(const MediaStream& stream)
{
	RunInMainThread([this, stream]() -> void {
		void* wnd = RenderMgr::Instance().FindRender(STRM_ID(stream));
		if (wnd == nullptr) {
			LOG_ERR("Get renderer window failed");
		}
		else {
			if (ERR_CODE_OK != m_rtc_engine->StopRecvVideo(stream, wnd)) {
				LOG_ERR("Stop recv video stream failed");
			}
			RenderMgr::Instance().RemoveRender(STRM_ID(stream));
		}
	});

	m_streams.erase(stream.stream.stream_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnPubStreamNotify(uint32_t group_id, const MediaStream& stream,
	bool operation)
{
	LOG_INF("OnPubStreamNotify, app:{}, group:{}, user:{}, stream:{}|{}, media:{}|{}",
		stream.src.app_id,
		group_id, 
		stream.src.user_id,
		stream.stream.stream_type,
		stream.stream.stream_id,
		stream.src.src_type,
		stream.src.src_id);

	if (STRM_TYPE(stream) == StreamType::VIDEO) {
		if (operation) {
			OnPubVideoNotify(group_id, stream);
		}
		else {
			OnUnpubVideoNotify(group_id, stream);
		}
	}
	else if (STRM_TYPE(stream) == StreamType::AUDIO) {
		if (operation) {
			OnPubAudioNotify(group_id, stream);
		}
		else {
			OnUnpubAudioNotify(group_id, stream);
		}
	}
	else {
		LOG_ERR("Invalid stream type:{}", STRM_TYPE(stream));
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnAudioDevEnergy(const std::string& dev_id, uint32_t energy)
{
	m_test_handler->OnAudioEnergy(m_test_mic, energy);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnJoinGroupNotify(uint32_t user_id, uint32_t group_id)
{
	RunInMainThread([this, user_id, group_id]() -> void {
		if (m_event_handler) {
			m_event_handler->OnJoinGroupNotify(user_id, group_id);
		}
	});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnLeaveGroupNotify(uint32_t user_id, uint32_t group_id)
{
	LOG_INF("User:{} leave group:{}", user_id, group_id);

	RunInMainThread([this, user_id, group_id]() -> void {
		if (m_event_handler) {
			m_event_handler->OnLeaveGroupNotify(user_id, group_id);
		}
	});

	bool exit = false;
	while (!exit) {
		exit = true;
		for (auto& item : m_streams) {
			if (item.second.src.user_id == user_id) {
				if (item.second.stream.stream_type == StreamType::AUDIO) {
					DoStopRecvAudio(item.second);
				}
				else if (item.second.stream.stream_type == StreamType::VIDEO) {
					DoStopRecvVideo(item.second);
				}
				exit = false;
				break;
			}
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnRunState(const std::string& desc)
{
	RunInMainThread([this, desc]() -> void {
		if (m_event_handler) {
			m_event_handler->OnRunState(desc);
		}
	});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnLoginResult(uint32_t user_id, ErrCode result)
{
	RunInMainThread([this, result]() -> void {
		if (m_event_handler) {
			m_event_handler->OnLoginResult(result == ERR_CODE_OK);
		}
	});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnLogoutResult(uint32_t user_id, ErrCode result)
{
	RunInMainThread([this, result]() -> void {
		if (m_event_handler) {
			m_event_handler->OnLogoutResult(result == ERR_CODE_OK);
		}
	});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnJoinGroupResult(uint32_t group_id, ErrCode result)
{
	RunInMainThread([this, result]() -> void {
		if (m_event_handler) {
			m_event_handler->OnJoinResult(result == ERR_CODE_OK);
		}
	});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnLeaveGroupResult(uint32_t group_id, ErrCode result)
{
	RunInMainThread([this, result]() -> void {
		if (m_event_handler) {
			m_event_handler->OnLeaveResult(result == ERR_CODE_OK);
		}
	});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SdkMgr::StartTestMic(const std::string& dev_id, const MicParam& param,
	IAudioTestHandler* handler)
{
	ErrCode result = m_rtc_engine->StartMicTest(dev_id);
	if (result != ERR_CODE_OK) {
		return false;
	}

	m_test_mic = dev_id;
	m_test_handler = handler;

	return result == ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::StopTestMic(const std::string& dev_id)
{
	m_rtc_engine->StopMicTest(dev_id);
	m_test_mic.clear();
	m_test_handler = nullptr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::StartPreviewCamera(const std::string& dev_id, const CamParam& param,
	void* wnd)
{
	//ErrCode result = m_rtc_engine->OpenCamera(dev_id, param);

	m_preview_wnd = wnd;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::StopPreviewCamera(const std::string& dev_id, void* wnd)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnAudioStreamStats(const com::MediaStream& stream,
	const com::AudioStreamStats& stats)
{
	LOG_INF("Audio stats, bitrate:{}, loss_rate:{}, rtt:{}",
		stats.bitrate, stats.loss_rate, stats.rtt);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SdkMgr::OnVideoStreamStats(const com::MediaStream& stream,
	const com::VideoStreamStats& stats)
{
	std::string render_id;
	if (stream.src.app_id != 0) { // remote
		render_id = stream.stream.stream_id;
	}
	else {
		render_id = stream.src.src_id;
	}

	std::ostringstream oss;
	oss << stats.width << "*" << stats.height
		<< "|" << stats.frame_rate
		<< "|" << stats.bitrate
		<< "|" << stats.loss_rate
		<< "|" << stats.rtt;

	m_event_handler->OnStreamStats(render_id, oss.str());
}

}