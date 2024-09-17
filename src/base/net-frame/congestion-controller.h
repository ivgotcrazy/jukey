#pragma once

#include "if-congestion-controller.h"
#include "if-session.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class CongestionController : public ICongestionController
{
public:
	CongestionController(const SessionParam& param);

	// ICongestionController
	virtual const CCParam& GetCCParam() override;
	virtual void UpdateRtt(uint32_t rtt) override;
	virtual void UpdateInflight(uint32_t inflight) override;
	virtual void UpdateRemoteWnd(uint32_t wnd) override;
	virtual void UpdateLinkCap(uint32_t pkt_rate) override;
	virtual void OnRecvAck(uint32_t sn, uint32_t count) override;
	virtual void OnRecvPkt(uint32_t sn) override {}
	virtual void OnPktLoss() override {}
	virtual void OnPktSent(uint32_t sn) override {}

private:
	const SessionParam& m_sess_param;

	uint32_t m_inflight = 0;

	uint32_t m_rtt = 0;

	// Packet rate of link
	uint32_t m_link_cap = 0;

	uint32_t m_remote_wnd = 16;

	CCParam m_cc_param;

	// Slow start threshold
	uint32_t m_ssthresh = 20;

	uint32_t m_fast_resend = 3;
};

}