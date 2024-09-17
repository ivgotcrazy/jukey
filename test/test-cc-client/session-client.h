// test-session-client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "if-client.h"
#include "if-session-mgr.h"
#include "thread/common-thread.h"
#include "common/util-common.h"
#include "if-stream-sender.h"
#include "if-timer-mgr.h"

using namespace jukey::net;
using namespace jukey::com;
using namespace jukey::txp;
using namespace jukey::util;

//==============================================================================
// 
//==============================================================================
class SessionClient 
	: public CommonThread
	, public IClient
	, public IStreamSenderHandler
{
public:
  SessionClient();

  // IClient
  virtual bool Init(const ClientParam& param) override;
  virtual bool Start() override;

  // CommonThread
  virtual void OnThreadMsg(const CommonMsg& msg);

	// ISenderHandler
	virtual void OnStreamData(uint32_t channel_id,
		uint32_t user_id,
		const MediaStream& stream,
		const Buffer& buf) override;
	virtual void OnSenderFeedback(uint32_t channel_id,
		uint32_t user_id,
		const MediaStream& stream,
		const Buffer& buf) override;
	virtual void OnBandwidthUpdate(uint32_t channel_id,
		uint32_t user_id,
		const MediaStream& stream,
		uint32_t bw_kbps) override;

private:
  void OnSessionCreateResult(const CommonMsg& msg);
  void OnSessionClosed();
	void OnSessionData(const CommonMsg& msg);
	void OnTimer();

private:
  ISessionMgr* m_sess_mgr = nullptr;
  Address m_addr;
  ClientParam m_client_param;
	IStreamSender* m_stream_sender = nullptr;
	TimerId m_timer_id = INVALID_TIMER_ID;
	uint32_t m_send_rate_kbps = 500;
	uint32_t m_send_seq = 0;
	uint32_t m_frame_seq = 0;
	SessionId m_session_id = INVALID_SESSION_ID;
};
