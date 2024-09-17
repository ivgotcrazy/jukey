#pragma once

#include "common-struct.h"
#include "session-base.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class ClientSession : public SessionBase
{
public:
	ClientSession(SessionMgr* mgr, 
		SessionThreadSP st, 
		const SessionParam& sparam,
		const CreateParam& cparam);
	~ClientSession();

	// SessionBase
	virtual void DoInit() override;
	virtual void DoClose() override;
	virtual void DoUpdate() override;
	virtual void OnSessionControlMsg(const SessionPktSP& pkt) override;

private:
	void OnHandshake(const SessionPktSP& sp);
	void OnReconnectRsp(const SessionPktSP& sp);
	void OnSessionClose(const SessionPktSP& sp);
	void OnKeepAlive(const SessionPktSP& sp);
	void SendHandshakeReq();
	void UpdateHandshakingState();
	void UpdateTransportingState();
	void UpdateReconnectingState();
	void NotifySessionCreateResult(bool result);

private:
	CreateParam m_create_param;

	uint64_t m_send_handshake_ts = 0;
	uint64_t m_last_reconnect_ts = 0;
	uint64_t m_last_send_reconnect_ts = 0;

	uint32_t m_handshake_retry_count = 0;
};

}