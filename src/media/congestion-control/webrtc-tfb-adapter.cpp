#include "webrtc-tfb-adapter.h"
#include "log.h"


using namespace jukey::base;

namespace jukey::cc
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
WebrtcTfbAdapter::WebrtcTfbAdapter(base::IComFactory* factory,
	const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_WEBRTC_TFB_ADAPTER, owner)
{
	
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* WebrtcTfbAdapter::CreateInstance(IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_WEBRTC_TFB_ADAPTER) == 0) {
		return new WebrtcTfbAdapter(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* WebrtcTfbAdapter::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_WEBRTC_TFB_ADAPTER)) {
		return static_cast<IWebrtcTfbAdapter*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void WebrtcTfbAdapter::Init(uint16_t base_seq, uint32_t base_ts_ms, uint8_t fb_sn)
{
	m_feedback.SetBase(base_seq, webrtc::Timestamp::Millis(base_ts_ms));
	m_feedback.SetFeedbackSequenceNumber(fb_sn);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool WebrtcTfbAdapter::AddReceivedPacket(uint16_t seq, uint32_t ts_ms)
{
	return m_feedback.AddReceivedPacket(seq, webrtc::Timestamp::Millis(ts_ms));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer WebrtcTfbAdapter::Serialize()
{
	size_t curr_pos = 0; // 当前写入位置
	uint32_t buf_len = (uint32_t)m_feedback.BlockLength();

	com::Buffer buf(buf_len, buf_len);

	if (!m_feedback.Create(DP(buf), &curr_pos, buf_len, nullptr)) {
		buf.data_len = 0;
		LOG_ERR("Serialize feedback failed!");
	}

	return buf;
}

}