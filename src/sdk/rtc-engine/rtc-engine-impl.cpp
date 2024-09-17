#include "rtc-engine-impl.h"
#include "stream-msg-builder.h"
#include "group-msg-builder.h"
#include "protoc/group.pb.h"
#include "protoc/transport.pb.h"
#include "protoc/terminal.pb.h"
#include "protoc/user.pb.h"
#include "log.h"
#include "common/util-net.h"
#include "common/util-common.h"
#include "common/util-pb.h"
#include "net-message.h"
#include "util.h"
#include "if-property.h"
#include "common-message.h"
#include "util-protocol.h"
#include "common/util-property.h"

using namespace jukey::com;

namespace jukey::sdk
{

RtcEngineImpl* RtcEngineImpl::s_rtc_engine = nullptr;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IRtcEngine* RtcEngineImpl::Instance()
{
	// TODO: thread safe
	if (!s_rtc_engine) {
		s_rtc_engine = new RtcEngineImpl();
	}
	return s_rtc_engine;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::Release()
{
	if (s_rtc_engine) {
		delete s_rtc_engine;
		s_rtc_engine = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RtcEngineImpl::RtcEngineImpl()
	: CommonThread("RTC engine", false)
	, m_login_processor(this)
	, m_join_processor(this)
	, m_camera_processor(this)
	, m_microphone_processor(this)
	, m_register_processor(this)
	, m_stream_processor(this)
	, m_media_file_processor(this)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::CheckParam(const RtcEngineParam& param)
{
	if (!param.handler) {
		LOG_ERR("Invalid handler!");
		return ERR_CODE_INVALID_PARAM;
	}

	if (!param.executor) {
		LOG_ERR("Invalid executor");
		return ERR_CODE_INVALID_PARAM;
	}

	if (param.com_path.empty()) {
		LOG_ERR("Empty component path!");
		return ERR_CODE_INVALID_PARAM;
	}

	if (param.address.empty()) {
		LOG_ERR("Empty address!");
		return ERR_CODE_INVALID_PARAM;
	}

	std::optional<com::Address> addr = util::ParseAddress(param.address);
	if (addr.has_value()) {
		m_proxy_addr = addr.value();
	}
	else {
		LOG_ERR("Invalid address:{}", param.address);
		return ERR_CODE_INVALID_PARAM;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::InitSessionMgr()
{
	net::CreateParam param;
	param.remote_addr  = m_proxy_addr;
	param.ka_interval  = 3; // second
	param.service_type = ServiceType::PROXY;
	param.session_type = net::SessionType::RELIABLE;
	param.thread       = this;

	m_proxy_session = m_media_engine->GetSessionMgr()->CreateSession(param);
	if (m_proxy_session == INVALID_SESSION_ID) {
		LOG_ERR("Create proxy session failed, addr:{}", m_proxy_addr.ToStr());
		return ERR_CODE_FAILED;
	}

	LOG_INF("Connecting to proxy service, addr:{}", m_proxy_addr.ToStr());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::Init(const RtcEngineParam& param)
{
	if (ERR_CODE_OK != CheckParam(param)) {
		LOG_ERR("Check rtc engine parameters failed!");
		return ERR_CODE_INVALID_PARAM;
	}
	m_engine_param = param;

	m_factory = GetComFactory();
	if (!m_factory) {
		LOG_ERR("Get component factory failed!");
		return ERR_CODE_FAILED;
	}

	if (!m_factory->Init(param.com_path.c_str())) {
		LOG_ERR("Init component factory failed!");
		return ERR_CODE_FAILED;
	}

	StartThread();

	m_media_engine = CreateMediaEngine();
	if (!m_media_engine) {
		LOG_ERR("Create media engine failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != m_media_engine->Init(m_factory, this, param.executor)) {
		LOG_ERR("Init media engine failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != InitSessionMgr()) {
		LOG_ERR("Initialize session manager failed!");
		return ERR_CODE_FAILED;
	}

	m_msg_builder.reset(new MsgBuilder(m_engine_param, m_engine_data));

	m_async_proxy.reset(new util::SessionAsyncProxy(m_factory, 
		m_media_engine->GetSessionMgr(), this, 10000));

	m_engine_state = RtcEngineState::INITED;

	// Wait register result
	bool result = m_init_promise.get_future().get();
	if (result) {
		m_engine_param.handler->OnRunState("Init rtc engine success");
		return ERR_CODE_OK;
	}
	else {
		m_engine_param.handler->OnRunState("Init rtc engine failed!");
		return ERR_CODE_FAILED;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::Login(uint32_t user_id)
{
	return m_login_processor.Login(user_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::Logout()
{
	return m_login_processor.Logout();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::JoinGroup(uint32_t group_id)
{
	return m_join_processor.JoinGroup(group_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::LeaveGroup()
{
	return m_join_processor.LeaveGroup();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::OpenCamera(const CamParam& param, void* wnd)
{
	return m_camera_processor.OpenCamera(param, wnd);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::CloseCamera(uint32_t dev_id)
{
	return m_camera_processor.CloseCamera(dev_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::OpenMic(const MicParam& param)
{
	return m_microphone_processor.OpenMicrophone(param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::CloseMic(uint32_t dev_id)
{
	return m_microphone_processor.CloseMicrophone(dev_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::OpenMediaFile(const std::string& file, void* wnd)
{
	return m_media_file_processor.Open(file, wnd);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::CloseMediaFile(const std::string& file)
{
	return m_media_file_processor.Close(file);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::StartRecvAudio(const MediaStream& stream)
{
	return m_stream_processor.StartRecvAudio(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::StopRecvAudio(const MediaStream& stream)
{
	return m_stream_processor.StopRecvAudio(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::StartRecvVideo(const MediaStream& stream, void* wnd)
{
	return m_stream_processor.StartRecvVideo(stream, wnd);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::StopRecvVideo(const MediaStream& stream, void* wnd)
{
	return m_stream_processor.StopRecvVideo(stream, wnd);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::StartMicTest(const std::string& dev_id)
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::StopMicTest(const std::string& dev_id)
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::StartRecord(const std::string& ouput_file)
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::StopRecord()
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IMediaDevMgr* RtcEngineImpl::GetMediaDevMgr()
{
	return m_media_engine->GetMediaDevMgr();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnAddMediaStream(const MediaStream& stream)
{
	LOG_INF("Add media stream, media:{}|{}, stream:{}|{}",
		stream.src.src_type, stream.src.src_id,
		stream.stream.stream_type, stream.stream.stream_id);

	if (stream.src.app_id != 0) { // remote
		m_stream_processor.OnAddMediaStream(stream);
	}
	else { // local
		if (stream.src.src_type == MediaSrcType::CAMERA) {
			m_camera_processor.OnAddCamStream(stream);
		}
		else if (stream.src.src_type == MediaSrcType::MICROPHONE) {
			m_microphone_processor.OnAddMicStream(stream);
		}
		else if (stream.src.src_type == MediaSrcType::FILE) {
			m_media_file_processor.OnAddMediaStream(stream);
		}
		else {
			LOG_ERR("Unsupported media src type:{}", stream.src.src_type);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnRemoveMediaStream(const MediaStream& stream)
{
	LOG_INF("Remove media stream, media:{}|{}, stream:{}|{}",
		stream.src.src_type, stream.src.src_id,
		stream.stream.stream_type, stream.stream.stream_id);

	if (stream.src.app_id != 0) { // remote
		m_stream_processor.OnAddMediaStream(stream);
	}
	else { // local
		if (stream.src.src_type == MediaSrcType::CAMERA) {
			m_camera_processor.OnDelCamStream(stream);
		}
		else if (stream.src.src_type == MediaSrcType::MICROPHONE) {
			m_microphone_processor.OnDelMicStream(stream);
		}
		else if (stream.src.src_type == MediaSrcType::FILE) {
			m_media_file_processor.OnDelMediaStream(stream);
		}
		else {
			LOG_ERR("Unsupported media src type:{}", stream.src.src_type);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnAudioStreamEnergy(const Stream& stream, uint32_t energy)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnPlayProgress(const com::MediaSrc& msrc, uint32_t prog)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnRunState(const std::string& desc)
{
	m_engine_param.handler->OnRunState(desc);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnAudioStreamStats(const com::MediaStream& stream,
	const com::AudioStreamStats& stats)
{
	m_engine_param.handler->OnAudioStreamStats(stream, stats);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnVideoStreamStats(const com::MediaStream& stream,
	const com::VideoStreamStats& stats)
{
	m_engine_param.handler->OnVideoStreamStats(stream, stats);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnJoinGroupNotify(const com::Buffer& buf)
{
	prot::JoinGroupNotify notify;
	if (!notify.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse join group notify failed!");
		return;
	}

	LOG_INF("Received join group notify:{}", util::PbMsgToJson(notify));

	if (notify.user_id() == m_engine_data.user_id) {
		LOG_INF("Self join notify, do nothing");
		return;
	}

	m_engine_param.handler->OnJoinGroupNotify(notify.user_id(), notify.group_id());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnSessionData(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionDataMsg);

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(data->buf);

	LOG_INF("Received session message, msg:{}, len:{}",
		prot::util::MSG_TYPE_STR(sig_hdr->mt), data->buf.data_len);

	prot::util::DumpSignalHeader(g_logger, sig_hdr);

	switch (sig_hdr->mt) {
	case prot::MSG_JOIN_GROUP_NOTIFY:
		OnJoinGroupNotify(data->buf);
		break;
	case prot::MSG_PUBLISH_MEDIA_NOTIFY:
		OnPublishMediaNotify(data->buf);
		break;
	case prot::MSG_UNPUBLISH_MEDIA_NOTIFY:
		OnUnpublishMediaNotify(data->buf);
		break;
	case prot::MSG_LOGIN_SEND_CHANNEL_NOTIFY:
		OnLoginSendChannelNotify(data->buf);
		break;
	case prot::MSG_LEAVE_GROUP_NOTIFY:
		OnUserLeaveGroupNotify(data->buf);
		break;
	default:
		if (!m_async_proxy->OnSessionMsg(data->lsid, data->buf)) {
			LOG_ERR("Unknown protocol type:{}", (uint16_t)sig_hdr->mt);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnSessionClosed(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionClosedMsg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnSessionCreateResult(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionCreateResultMsg);

	if (!data->result) {
		LOG_ERR("Create session failed!");
		m_init_promise.set_value(false);
		return;
	}

	LOG_INF("Create session success, lsid:{}, rsid:{}, addr:{}", data->lsid,
		data->rsid, data->addr.ToStr());

	m_register_processor.Register();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnPublishMediaNotify(const Buffer& buf)
{
	prot::PublishMediaNotify notify;
	if (!notify.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse publish media notify failed!");
		return;
	}

	LOG_INF("Received publish media notify:{}", util::PbMsgToJson(notify));

	if (notify.user_id() == m_engine_data.user_id) {
		LOG_INF("Self publish, ignore it");
		return;
	}

	com::MediaStream stream;
	MAPP_ID(stream)   = notify.app_id();
	MUSER_ID(stream)  = notify.user_id();
	STRM_ID(stream)   = notify.media_entry().stream_id();
	STRM_TYPE(stream) = (StreamType)notify.media_entry().stream_type();
	MSRC_ID(stream)   = notify.media_entry().media_src_id();
	MSRC_TYPE(stream) = (MediaSrcType)notify.media_entry().media_src_type();

	m_engine_param.handler->OnPubStreamNotify(notify.group_id(), stream, true);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnUnpublishMediaNotify(const Buffer& buf)
{
	prot::UnpublishMediaNotify notify;
	if (!notify.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse unpublish media notify failed!");
		return;
	}

	LOG_INF("Received unpublish media notify:{}", util::PbMsgToJson(notify));

	if (notify.user_id() == m_engine_data.user_id) {
		LOG_INF("Self publish, ignore it");
		return;
	}

	com::MediaStream stream;
	MAPP_ID(stream) = notify.app_id();
	MUSER_ID(stream) = notify.user_id();
	STRM_ID(stream) = notify.media_entry().stream_id();
	STRM_TYPE(stream) = (StreamType)notify.media_entry().stream_type();
	MSRC_TYPE(stream) = (MediaSrcType)notify.media_entry().media_src_type();
	MSRC_ID(stream) = notify.media_entry().media_src_id();

	m_engine_param.handler->OnPubStreamNotify(notify.group_id(), stream, false);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::SendLoginSendChannelAck(const Buffer& buf,
	const prot::LoginSendChannelNotify& notify)
{
	com::Buffer send_buf = m_msg_builder->BuildLoginSendChannelAck(buf, notify);

	if (ERR_CODE_OK != m_media_engine->GetSessionMgr()->SendData(
		m_proxy_session, send_buf)) {
		LOG_ERR("Send login send channel ack failed");
	}
	else {
		LOG_INF("Send login send channel ack");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::StartSendStream(const prot::LoginSendChannelNotify& notify)
{
	com::MediaStream media_stream = util::ToMediaStream(notify.stream());

	ErrCode result = m_media_engine->StartSendStream(media_stream, notify.stream_addr());
	if (result != ERR_CODE_OK) {
		LOG_ERR("Start send stream failed");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnLoginSendChannelNotify(const Buffer& buf)
{
	prot::LoginSendChannelNotify notify;
	if (!notify.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse login send channel notify failed!");
		return;
	}

	LOG_INF("Received login send channel notify:{}", util::PbMsgToJson(notify));

	SendLoginSendChannelAck(buf, notify);

	StartSendStream(notify); // TODO: 应该是通知发送的时候再发送
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnUserLeaveGroupNotify(const Buffer& buf)
{
	prot::LeaveGroupNotify notify;
	if (!notify.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse user leave group notify failed!");
		return;
	}

	LOG_INF("Received user leave group notify:{}", util::PbMsgToJson(notify));

	// 停止接收用户的所有媒体
	m_engine_param.handler->OnLeaveGroupNotify(notify.user_id(), notify.group_id());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnPubStreamRsp(const com::MediaStream& stream,
	const com::Buffer& buf)
{
	prot::PublishStreamRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse publish stream response failed!");
		return;
	}

	LOG_INF("Received publish stream response:{}", util::PbMsgToJson(rsp));

	SendPublishMediaReq(stream);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnPubStreamTimeout(const com::MediaStream& stream)
{
	LOG_ERR("Receive publish stream:{}|{} response failed!",
		stream.stream.stream_type, stream.stream.stream_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnPubStreamError(const com::MediaStream& stream, com::ErrCode ec)
{
	LOG_ERR("Send publish stream:{}|{} request error:{}",
		stream.stream.stream_type, stream.stream.stream_id, ec);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::PublishStream(const com::MediaStream& stream)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	com::Buffer buf = m_msg_builder->BuildPubStreamReq(stream, ++m_cur_seq);

	m_async_proxy->SendSessionMsg(m_proxy_session, buf, m_cur_seq,
		prot::MSG_PUBLISH_STREAM_RSP)
		.OnResponse([this, stream](net::SessionId sid, const com::Buffer& buf) {
			OnPubStreamRsp(stream, buf);
		}).OnTimeout([this, stream]() {
			OnPubStreamTimeout(stream);
		}).OnError([this, stream](const std::string& err) {
			OnPubStreamError(stream, ERR_CODE_FAILED);
		});

	LOG_INF("Send publish stream request, app:{}, user:{}, client:{}, seq:{}, "
		"media:{}|{}, stream:{}|{}",
		m_engine_param.app_id,
		m_engine_data.user_id,
		m_engine_param.client_id,
		m_cur_seq,
		stream.src.src_type,
		stream.src.src_id,
		stream.stream.stream_type,
		stream.stream.stream_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::SendPublishMediaReq(const MediaStream& stream)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	com::Buffer buf = m_msg_builder->BuildPubMediaReq(stream, ++m_cur_seq);

	m_async_proxy->SendSessionMsg(m_proxy_session, buf, m_cur_seq,
		prot::MSG_PUBLISH_MEDIA_RSP)
		.OnResponse([this, stream](net::SessionId sid, const com::Buffer& buf) {
			OnPubMediaRsp(stream, buf);
		}).OnTimeout([this, stream]() {
			OnPubMediaTimeout(stream);
		}).OnError([this, stream](const std::string& err) {
			OnPubMediaError(stream, ERR_CODE_FAILED);
		});

	LOG_INF("Send publish media request, app:{}, group:{}, user:{}, client:{}, "
		"seq:{}, stream:{}|{}, media:{}|{}",
		m_engine_param.app_id,
		m_engine_data.group_id,
		m_engine_data.user_id,
		m_engine_param.client_id,
		m_cur_seq,
		(uint32_t)stream.stream.stream_type,
		stream.stream.stream_id,
		(uint32_t)stream.src.src_type,
		stream.src.src_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnUnpubMediaRsp(const MediaStream& stream, const Buffer& buf)
{
	
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnUnpubMediaTimeout(const MediaStream& stream)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnUnpubMediaError(const MediaStream& stream, ErrCode ec)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::SendUnpublishMediaReq(const MediaStream& stream)
{
	com::Buffer buf = m_msg_builder->BuildUnpubMediaReq(stream, ++m_cur_seq);

	m_async_proxy->SendSessionMsg(m_proxy_session, buf, m_cur_seq,
		prot::MSG_UNPUBLISH_MEDIA_RSP)
		.OnResponse([this, stream](net::SessionId sid, const com::Buffer& buf) {
			OnUnpubMediaRsp(stream, buf);
		}).OnTimeout([this, stream]() {
			OnUnpubMediaTimeout(stream);
		}).OnError([this, stream](const std::string& err) {
			OnUnpubMediaError(stream, ERR_CODE_FAILED);
		});

	LOG_INF("Send unpublish media request, app:{}, group:{}, user:{}, client:{}, "
		"seq:{}, stream:{}|{}, media:{}|{}",
		m_engine_param.app_id,
		m_engine_data.group_id,
		m_engine_data.user_id,
		m_engine_param.client_id,
		m_cur_seq,
		(uint32_t)stream.stream.stream_type,
		stream.stream.stream_id,
		(uint32_t)stream.src.src_type,
		stream.src.src_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::PublishGroupStream(const MediaStream& stream)
{
	PublishStream(stream);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnPubMediaRsp(const MediaStream& stream, const Buffer& buf)
{
	prot::PublishMediaRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse publish group stream response failed!");
		return;
	}

	LOG_INF("Received publish media response:{}", util::PbMsgToJson(rsp));

	// TODO: open media src callback!!!
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnPubMediaTimeout(const com::MediaStream& stream)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnPubMediaError(const com::MediaStream& stream,
	com::ErrCode ec)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnUnpubStreamRsp(const MediaStream& stream, const Buffer& buf)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnUnpubStreamTimeout(const MediaStream& stream)
{
	LOG_ERR("Receive unpublish stream:{}|{} response failed!",
		stream.stream.stream_type, stream.stream.stream_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnUnpubStreamError(const MediaStream& stream, ErrCode ec)
{
	LOG_ERR("Send unpublish stream:{}|{} request error:{}",
		stream.stream.stream_type, stream.stream.stream_id, ec);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::UnpublishStream(const com::MediaStream& stream)
{
	com::Buffer buf = m_msg_builder->BuildUnpubStreamReq(stream, ++m_cur_seq);

	m_async_proxy->SendSessionMsg(m_proxy_session, buf, m_cur_seq,
		prot::MSG_UNPUBLISH_STREAM_RSP)
		.OnResponse([this, stream](net::SessionId sid, const com::Buffer& buf) {
			OnUnpubStreamRsp(stream, buf);
		}).OnTimeout([this, stream]() {
			OnUnpubStreamTimeout(stream);
		}).OnError([this, stream](const std::string& err) {
			OnUnpubStreamError(stream, ERR_CODE_FAILED);
		});

	LOG_INF("Send unpublish stream request, app:{}, user:{}, client:{}, seq:{}, "
		"media:{}|{}, stream:{}|{}",
		m_engine_param.app_id,
		m_engine_data.user_id,
		m_engine_param.client_id,
		m_cur_seq,
		stream.src.src_type,
		stream.src.src_id,
		stream.stream.stream_type,
		stream.stream.stream_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RtcEngineImpl::UnpublishGroupStream(const com::MediaStream& stream)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	// 分组中取消发布媒体
	SendUnpublishMediaReq(stream);

	// 取消发布流
	UnpublishStream(stream);

	// 停止发送流
	m_media_engine->StopSendStream(stream);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcEngineImpl::OnThreadMsg(const com::CommonMsg& msg)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	switch (msg.msg_type) {
	case net::NET_MSG_SESSION_CLOSED:
		OnSessionClosed(msg);
		break;
	case net::NET_MSG_SESSION_CREATE_RESULT:
		OnSessionCreateResult(msg);
		break;
	case net::NET_MSG_SESSION_DATA:
		OnSessionData(msg);
		break;
	case net::NET_MSG_SESSION_INCOMING:
		break; // noting to do
	default:
		LOG_ERR("Unknown message type:{}", msg.msg_type);
	}
}

}