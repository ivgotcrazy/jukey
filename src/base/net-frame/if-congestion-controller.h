#pragma once

#include <inttypes.h>
#include <memory>

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
struct CCParam
{
	uint32_t cwnd = 0;
	uint32_t pacing_rate = 0;
};

//==============================================================================
// 
//==============================================================================
class ICongestionController
{
public:
	virtual ~ICongestionController() {}

	virtual const CCParam& GetCCParam() = 0;

	virtual void UpdateRtt(uint32_t rtt) = 0;

	virtual void UpdateInflight(uint32_t inflight) = 0;

	virtual void UpdateRemoteWnd(uint32_t wnd) = 0;

	virtual void UpdateLinkCap(uint32_t pkt_rate) = 0;

	virtual void OnRecvAck(uint32_t sn, uint32_t count) = 0;

	virtual void OnRecvPkt(uint32_t sn) = 0;

	virtual void OnPktLoss() = 0;

	virtual void OnPktSent(uint32_t sn) = 0;
};
typedef std::unique_ptr<ICongestionController> ICongestionControllerUP;

}