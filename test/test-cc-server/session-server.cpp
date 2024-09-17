// test-session-server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <iostream>

#include "session-server.h"
#include "com-factory.h"
#include "net-message.h"
#include "common/util-net.h"
#include "common/util-time.h"
#include "common-define.h"
#include "net-message.h"
#include "protocol.h"

using namespace jukey::base;
using namespace jukey::prot;
using namespace std::placeholders;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionServer::SessionServer() : CommonThread("session server", true) 
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionServer::Init(const std::string& addr, uint32_t log_level)
{
	std::optional<Address> result = ParseAddress(addr);
	if (result.has_value()) {
		m_addr = result.value();
	}
	else {
		std::cout << "Invalid address: " << addr << std::endl;
		return false;
	}

	IComFactory* factory = GetComFactory();
	if (!factory) {
		std::cout << "Get component factory failed!" << std::endl;
		return false;
	}

	if (!factory->Init("./")) {
		std::cout << "Init component factory failed!" << std::endl;
		return false;
	}

	m_sess_mgr = (ISessionMgr*)GetComFactory()->QueryInterface(CID_SESSION_MGR,
		IID_SESSION_MGR, "test");
	if (!m_sess_mgr) {
		std::cout << "Create session manager failed!" << std::endl;
		return false;
	}

	SessionMgrParam param;
	param.thread_count = 4;
	param.ka_interval = 5;

	if (ERR_CODE_OK != m_sess_mgr->Init(param)) {
		std::cout << "Init session manager failed!" << std::endl;
		return false;
	}

	m_sess_mgr->SetLogLevel(log_level);	

	return true;
}
	
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionServer::StartStreamReceiver()
{
	m_receiver = (IStreamReceiver*)GetComFactory()->QueryInterface(
		CID_STREAM_RECEIVER, IID_STREAM_RECEIVER, "test");
	if (!m_receiver) {
		std::cout << "Create stream receiver failed!" << std::endl;
		return;
	}

	MediaStream stream;
	stream.src.app_id = 1;
	stream.src.user_id = 1;
	stream.src.src_type = MediaSrcType::CAMERA;
	stream.src.src_id = "test";
	stream.stream.stream_type = StreamType::VIDEO;
	stream.stream.stream_id = "test_stream";
	if (ERR_CODE_OK != m_receiver->Init(this, 1, 1, stream)) {
		std::cout << "Init stream receiver failed!" << std::endl;
		return;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionServer::Start()
{
	ListenParam param;
	param.listen_addr = m_addr;
	param.listen_srv = ServiceType::NONE;
	param.thread = this;
	ListenId listen_id = m_sess_mgr->AddListen(param);
	if (listen_id == INVALID_LISTEN_ID) {
		printf("Add listen failed!\n");
		return false;
	}

	StartThread();

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionServer::OnSessionData(const jukey::com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(SessionDataMsg);

	// 统计
	m_recv_data_len += data->buf.data_len;
	m_recv_data_count++;

	SigMsgHdr* sig_hdr = (SigMsgHdr*)DP(data->buf);

	// 去除 SigMsgHdr 头
	uint32_t buf_len = data->buf.data_len - sizeof(SigMsgHdr);
	Buffer msg_buf(buf_len, buf_len);
	memcpy(DP(msg_buf), DP(data->buf) + sizeof(SigMsgHdr), buf_len);
	
	if (sig_hdr->mt == jukey::prot::MSG_STREAM_DATA) {
		m_receiver->InputStreamData(msg_buf);
	}
	else if (sig_hdr->mt == jukey::prot::MSG_STREAM_FEEDBACK) {
		m_receiver->InputFeedback(msg_buf);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionServer::OnSessionClosed(const CommonMsg& msg)
{
	printf(">>> Receive packet count: %lld, packet size: %lld\n",
		m_recv_data_count, m_recv_data_len);
	m_recv_data_len = 0;
	m_recv_data_count = 0;

	if (m_receiver) {
		m_receiver->Release();
		m_receiver = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionServer::OnSessionIncoming(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(SessionIncommingMsg);

	m_session_id = data->lsid;

	StartStreamReceiver();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionServer::OnThreadMsg(const jukey::com::CommonMsg& msg)
{
	switch (msg.msg_type) {
	case jukey::net::NET_MSG_SESSION_DATA:
		OnSessionData(msg);
		break;
	case jukey::net::NET_MSG_SESSION_CLOSED:
		OnSessionClosed(msg);
		break;
	case jukey::net::NET_MSG_SESSION_INCOMING:
		OnSessionIncoming(msg);
		break;
	default:
		printf("msg type: %d\n", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionServer::OnStreamFrame(uint32_t channel_id, uint32_t user_id,
	const MediaStream& stream, const Buffer& buf)
{
	m_recv_frame_count++;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionServer::OnReceiverFeedback(uint32_t channel_id, uint32_t user_id,
	const MediaStream& stream, const Buffer& buf)
{
	// 添加 SigMsgHdr
	uint32_t buf_len = buf.data_len + sizeof(SigMsgHdr);
	Buffer sig_buf(buf_len, buf_len);

	SigMsgHdr* sig_hdr = (SigMsgHdr*)DP(sig_buf);
	sig_hdr->len = buf.data_len;
	sig_hdr->mt = jukey::prot::MSG_STREAM_FEEDBACK;
	sig_hdr->seq = ++m_cur_seq;
	sig_hdr->usr = user_id;
	memcpy(DP(sig_buf) + sizeof(SigMsgHdr), DP(buf), buf.data_len);

	m_sess_mgr->SendData(m_session_id, sig_buf);
}
