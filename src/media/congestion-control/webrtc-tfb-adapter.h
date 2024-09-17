#pragma once

#include "common-struct.h"
#include "if-congestion-controller.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"

#include "modules/rtp_rtcp/source/rtcp_packet/transport_feedback.h"

namespace jukey::cc
{

//==============================================================================
// 
//==============================================================================
class WebrtcTfbAdapter
	: public IWebrtcTfbAdapter
	, public base::ProxyUnknown
	, public base::ComObjTracer
{
public:
	WebrtcTfbAdapter(base::IComFactory* factory, const char* owner);

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	virtual void Init(uint16_t base_seq, uint32_t base_ts_ms, uint8_t fb_sn) override;
	virtual bool AddReceivedPacket(uint16_t seq, uint32_t ts_ms) override;
	virtual com::Buffer Serialize() override;

private:
	webrtc::rtcp::TransportFeedback m_feedback;
};

}