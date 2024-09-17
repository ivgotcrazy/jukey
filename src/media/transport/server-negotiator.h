#pragma once

#include "if-pin.h"
#include "transport-common.h"
#include "protoc/transport.pb.h"
#include "protocol.h"
#include "if-timer-mgr.h"

namespace jukey::txp
{

class StreamServer;

//==============================================================================
// 
//==============================================================================
class ServerNegotiator : public std::enable_shared_from_this<ServerNegotiator>
{
public:
	ServerNegotiator(StreamServer& server);
	~ServerNegotiator();
	
	void OnRecvNegoReq(uint32_t channel_id, const com::Buffer& buf);
	void OnRecvNegoRsp(uint32_t channel_id, const com::Buffer& buf);

	void OnNegoTimer();

private:
	void ProcRecverNegoReq(prot::SigMsgHdr* sig_hdr,
		const prot::NegotiateReq& req);

	void RecverNegoReqOnInitState(prot::SigMsgHdr* sig_hdr,
		const prot::NegotiateReq& req);
	void RecverNegoReqOnWaitState(prot::SigMsgHdr* sig_hdr,
		const prot::NegotiateReq& req);
	void RecverNegoReqOnDoneState(prot::SigMsgHdr* sig_hdr,
		const prot::NegotiateReq& req);

	void ProcSenderNegoReq(uint32_t channel_id,
		prot::SigMsgHdr* sig_hdr,
		const prot::NegotiateReq& req);

	void SenderNegoReqOnInitState(SenderInfoSP info,
		prot::SigMsgHdr* sig_hdr,
		const prot::NegotiateReq& req);
	void SenderNegoReqOnWaitState(SenderInfoSP info,
		prot::SigMsgHdr* sig_hdr,
		const prot::NegotiateReq& req);
	void SenderNegoReqOnDoneState(SenderInfoSP info,
		prot::SigMsgHdr* sig_hdr,
		const prot::NegotiateReq& req);

	void SendNegoRsp(uint32_t channel_id, 
		prot::SigMsgHdr* sig_hdr,
		com::ErrCode result, 
		const std::string& msg, 
		const std::string& cap);

	std::string DoNegotiate();

private:
	StreamServer& m_stream_server;
	com::TimerId m_timer_id = INVALID_TIMER_ID;
};
typedef std::shared_ptr<ServerNegotiator> ServerNegotiatorSP;

}