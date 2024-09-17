// test-session-client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <thread>

#include "session-client.h"
#include "net-message.h"
#include "common/util-net.h"
#include "common/util-common.h"
#include "common/util-time.h"
#include "event/common-event.h"
#include "protocol.h"
#include "log/util-log.h"

using namespace jukey::base;
using namespace jukey::util;
using namespace jukey::prot;

#define TIMEOUT_MS 20

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionClient::SessionClient() : CommonThread("session client", true)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionClient::Init(const ClientParam& param)
{
	std::optional<Address> result = ParseAddress(param.addr);
	if (result.has_value()) {
		m_addr = result.value();
	}
	else {
		std::cout << "Invalid address: " << param.addr << std::endl;
		return false;
	}

	m_client_param = param;

	IComFactory* factory = GetComFactory(); 
	if (!factory) {
		std::cout << "Get component factory failed!" << std::endl;
		return false;
	}

	if (!factory->Init("./")) {
		std::cout << "Init component factory failed!" << std::endl;
		return false;
	}

	m_sess_mgr = (ISessionMgr*)factory->QueryInterface(CID_SESSION_MGR,
		IID_SESSION_MGR, "test");
	if (!m_sess_mgr) {
		std::cout << "Create session manager failed!" << std::endl;
		return false;
	}

	SessionMgrParam mgr_param;
	mgr_param.thread_count = 4;
	mgr_param.ka_interval = 3;

	if (ERR_CODE_OK != m_sess_mgr->Init(mgr_param)) {
		std::cout << "Init session manager failed!" << std::endl;
		return false;
	}

	m_sess_mgr->SetLogLevel(m_client_param.log_level);

	//////////////////////////////////////////////////////////////////////////////

	m_stream_sender = (IStreamSender*)GetComFactory()->QueryInterface(
		CID_STREAM_SENDER, IID_STREAM_SENDER, "test");
	if (!m_stream_sender) {
		std::cout << "Create stream sender failed!" << std::endl;
		return false;
	}

	MediaStream stream;
	stream.src.app_id = 1;
	stream.src.user_id = 1;
	stream.src.src_type = MediaSrcType::CAMERA;
	stream.src.src_id = "test";
	stream.stream.stream_type = StreamType::VIDEO;
	stream.stream.stream_id = "test_stream";
	if (ERR_CODE_OK != m_stream_sender->Init(this, 1, 1, stream)) {
		std::cout << "Init stream sender failed!" << std::endl;
		return false;
	}
	BitrateConfig config;
	config.max_bitrate_kbps = 10'000;
	config.min_bitrate_kbps = 100;
	config.start_bitrate_kbps = 1'000;
	if (!m_stream_sender->SetBitrateConfig(config)) {
		std::cout << "Set bitrate config failed!" << std::endl;
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////

	ITimerMgr* timer_mgr = QUERY_TIMER_MGR(GetComFactory());
	assert(timer_mgr);

	TimerParam timer_param;
	timer_param.timeout = TIMEOUT_MS;
	timer_param.timer_type = TimerType::TIMER_TYPE_LOOP;
	timer_param.timer_name = "test-cc-client";
	timer_param.timer_func = [this](int64_t) { OnTimer(); };

	m_timer_id = timer_mgr->AllocTimer(timer_param);
	timer_mgr->StartTimer(m_timer_id);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionClient::OnTimer()
{
	uint32_t send_bytes = m_send_rate_kbps * TIMEOUT_MS / 8;

	Buffer buf(send_bytes, send_bytes);

	VideoFrameHdr* hdr = (VideoFrameHdr*)DP(buf);
	hdr->fseq = m_frame_seq++;

	//printf("<=== send_bytes:%d\n", send_bytes);

	m_stream_sender->InputFrameData(buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionClient::Start()
{
	StartThread();

	CreateParam param;
	param.remote_addr = m_addr;
	param.ka_interval = 3; // second
	param.service_type = ServiceType::NONE;
	param.session_type = SessionType::UNRELIABLE;
	param.thread = this;
	param.fec_type = FecType::NONE;

	SessionId sid = m_sess_mgr->CreateSession(param);
	if (sid == INVALID_SESSION_ID) {
		printf("Create session failed!\n");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionClient::OnSessionData(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(SessionDataMsg);

	SigMsgHdr* sig_hdr = (SigMsgHdr*)DP(data->buf);

	// 去除 SigMsgHdr 头
	uint32_t buf_len = data->buf.data_len - sizeof(SigMsgHdr);
	Buffer msg_buf(buf_len, buf_len);
	memcpy(DP(msg_buf), DP(data->buf) + sizeof(SigMsgHdr), buf_len);

	if (sig_hdr->mt == jukey::prot::MSG_STREAM_FEEDBACK) {
		m_stream_sender->InputFeedbackData(msg_buf);
	}
	else {
		printf("Unknown msg type:%d!\n", sig_hdr->mt);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionClient::OnThreadMsg(const CommonMsg& msg)
{
	switch (msg.msg_type) {
	case NET_MSG_SESSION_CREATE_RESULT:
		OnSessionCreateResult(msg);
		break;
	case NET_MSG_SESSION_CLOSED:
		OnSessionClosed();
		break;
	case NET_MSG_SESSION_DATA:
		OnSessionData(msg);
		break;
	default:
		printf("Unknown message type:%d\n", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionClient::OnSessionCreateResult(const jukey::com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(SessionCreateResultMsg);
	if (data->result) {
		printf("Session create success\n");
		m_session_id = data->lsid;
	}
	else {
		printf("Session create failed!\n");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionClient::OnSessionClosed()
{
	printf("Session closed!\n");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionClient::OnStreamData(uint32_t channel_id, uint32_t user_id,
	const MediaStream& stream, const Buffer& buf)
{
	// 添加 SigMsgHdr
	uint32_t buf_len = buf.data_len + sizeof(SigMsgHdr);
	Buffer sig_buf(buf_len, buf_len);

	SigMsgHdr* sig_hdr = (SigMsgHdr*)DP(sig_buf);
	sig_hdr->len = buf.data_len;
	sig_hdr->mt = jukey::prot::MSG_STREAM_DATA;
	sig_hdr->seq = ++m_send_seq;
	sig_hdr->usr = user_id;
	memcpy(DP(sig_buf) + sizeof(SigMsgHdr), DP(buf), buf.data_len);

	m_sess_mgr->SendData(m_session_id, sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionClient::OnSenderFeedback(uint32_t channel_id, uint32_t user_id,
	const MediaStream& stream, const Buffer& buf)
{
	// 添加 SigMsgHdr
	uint32_t buf_len = buf.data_len + sizeof(SigMsgHdr);
	Buffer sig_buf(buf_len, buf_len);

	SigMsgHdr* sig_hdr = (SigMsgHdr*)DP(sig_buf);
	sig_hdr->len = buf.data_len;
	sig_hdr->mt = jukey::prot::MSG_STREAM_FEEDBACK;
	sig_hdr->seq = ++m_send_seq;
	sig_hdr->usr = user_id;
	memcpy(DP(sig_buf) + sizeof(SigMsgHdr), DP(buf), buf.data_len);

	m_sess_mgr->SendData(m_session_id, sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionClient::OnBandwidthUpdate(uint32_t channel_id, uint32_t user_id,
	const MediaStream& stream, uint32_t bw_kbps)
{
	UTIL_INF("Update bandwidth from {} to {}", m_send_rate_kbps, bw_kbps);

	m_send_rate_kbps = bw_kbps;

	//m_send_rate_kbps = 100;
}