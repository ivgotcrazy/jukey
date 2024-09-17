#pragma once

#include "session-protocol.h"
#include "net-public.h"
#include "if-session.h"
#include "session-thread.h"
#include "if-session-processor.h"
#include "if-session-pkt-assembler.h"
#include "net-common.h"
#include "common/util-stats.h"
#include "session-pkt-sender.h"
#include "if-sending-queue.h"
#include "session-receiver.h"
#include "session-sender.h"
#include "link-cap-estimator.h"
#include "fec-pkt-assembler.h"

namespace jukey::net
{

class SessionMgr;
//==============================================================================
// 
//==============================================================================
class SessionBase 
	: public ISession
	, public ISendEntry
	, public ISendPktNotify
	, public std::enable_shared_from_this<SessionBase>
{
public:
	SessionBase(SessionMgr* mgr, SessionThreadSP st, const SessionParam& param);
	~SessionBase();

	// ISession
	virtual void Init() override;
	virtual void Close(bool active) override;
	virtual void OnUpdate() override;
	virtual void OnRecvData(const com::Buffer& buf) override;
	virtual bool OnSendData(const com::Buffer& buf) override;
	virtual const SessionParam& GetParam() override;

	// ISendEnetry
	virtual uint64_t GetEntryId() override;
	virtual uint64_t NextSendTime() override;
	virtual void SendData() override;

	// ISendPktNotify
	virtual void OnSendPkt(uint32_t data_type, uint32_t data_len) override;

protected:
	void CreateObjects();
	void SendHandshake(const HandshakeData& data);
	void SendKeepAlive();
	void SendSessionClose();
	void SendReconnectReq();
	void SendReconnectRsp();
	void OnSessionData(const SessionPktSP& pkt);
	void OnSessionNegotiateCompelete();
	void OnRecvSessionPkt(const SessionPktSP& pkt);
	void OnRecvFecProtocolData(const com::Buffer& buf);
	SessionPktSP ConvertSessionPkt(const com::Buffer& buf);
	void ProcSessionDataPkt(const SessionPktSP& pkt);

	// Derived class implementes
	virtual void DoInit() = 0;
	virtual void DoClose() = 0;
	virtual void DoUpdate() = 0;
	virtual void OnSessionControlMsg(const SessionPktSP& pkt) = 0;

protected:
	SessionMgr* m_sess_mgr = nullptr;
	base::IComFactory* m_factory = nullptr;

	ISessionPktSenderSP m_pkt_sender;
	
	SessionThreadSP m_sess_thread;

	IRttFilterSP m_rtt_filter;
	LinkCapEstimatorSP m_link_cap_estimator;

	SessionParam m_sess_param;
	SessionState m_sess_state = SESSION_STATE_INVALID;

	ISessionPktAssemblerSP m_sess_pkt_assembler;
	FecPktAssemblerSP      m_fec_pkt_assembler;

	SessionReceiverUP m_sess_recver;
	SessionSenderUP   m_sess_sender;

	uint64_t m_last_recv_data_ts = 0;
	uint64_t m_last_send_data_ts = 0;

	uint64_t m_last_recv_psn = 0;
};
typedef std::shared_ptr<SessionBase> SessionBaseSP;

}