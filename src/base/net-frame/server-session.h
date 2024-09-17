#pragma once

#include "session-base.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class ServerSession : public SessionBase
{
public:
	ServerSession(SessionMgr* mgr, 
		SessionThreadSP t, 
		const SessionParam& sparam,
		const SessionMgrParam& mparam);
	~ServerSession();

	// SessionBase
	virtual void DoInit() override;
	virtual void DoClose() override;
	virtual void DoUpdate() override;
	virtual void OnSessionControlMsg(const SessionPktSP& pkt) override;

private:
	void OnHandshake(const SessionPktSP& pkt);
	void OnReconnectReq(const SessionPktSP& pkt);
	void OnSessionClose(const SessionPktSP& pkt);
	void OnKeepAlive(const SessionPktSP& pkt);
	bool CheckHandshakeData(const HandshakeData& data);
	void SendHandshakeRsp();
	void UpdateHandshakingState();
	void UpdateTransportingState();
	void UpdateReconnectingState();

private:
	SessionMgrParam m_mgr_param;

	uint64_t m_wait_handshake_ts = 0;
	uint64_t m_wait_reconnect_ts = 0;
};

}