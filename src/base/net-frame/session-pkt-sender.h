#include "if-session-pkt-sender.h"
#include "if-tcp-mgr.h"
#include "if-udp-mgr.h"
#include "if-session.h"
#include "common/util-stats.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class SessionPktSender : public ISessionPktSender
{
public:
	SessionPktSender(base::IComFactory* factory,
		ITcpMgr* tcp_mgr,
		IUdpMgr* udp_mgr, 
		ISendPktNotify* notify,
		const SessionParam& param);

	// ISessionPktSender
	virtual com::ErrCode SendPkt(uint32_t data_type, const com::Buffer& buf) override;

private:
	ITcpMgr* m_tcp_mgr = nullptr;
	IUdpMgr* m_udp_mgr = nullptr;
	SessionParam m_sess_param;
	ISendPktNotify* m_notify = nullptr;
};

}