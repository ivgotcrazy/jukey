// test-session-server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <iostream>

#include "if-server.h"
#include "if-session-mgr.h"
#include "thread/common-thread.h"
#include "common/util-net.h"
#include "if-stream-receiver.h"

using namespace jukey::net;
using namespace jukey::util;
using namespace jukey::com;
using namespace jukey::txp;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
class SessionServer 
	: public CommonThread
	, public IServer
	, public IStreamReceiverHandler
{
public:
	SessionServer();

	// IServer
	virtual bool Init(const std::string& addr, uint32_t log_level) override;
	virtual bool Start() override;

	// CommonThread
	virtual void OnThreadMsg(const CommonMsg& msg);

	// IReceiverHandler
	virtual void OnStreamFrame(uint32_t channel_id,
		uint32_t user_id,
		const MediaStream& stream, 
		const Buffer& buf) override;
	virtual void OnReceiverFeedback(uint32_t channel_id, 
		uint32_t user_id,
		const MediaStream& stream, 
		const Buffer& buf) override;

private:
	void OnSessionData(const CommonMsg& msg);
	void OnSessionClosed(const CommonMsg& msg);
	void OnSessionIncoming(const CommonMsg& msg);
	void StartStreamReceiver();

private:
	ISessionMgr* m_sess_mgr = nullptr;
	IStreamReceiver* m_receiver = nullptr;

	Address m_addr;
	uint64_t m_recv_data_len = 0;
	uint64_t m_recv_data_count = 0;

	uint64_t m_recv_frame_count = 0;

	uint32_t m_cur_seq = 0;

	SessionId m_session_id = INVALID_SESSION_ID;
};
