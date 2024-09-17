#include "mq-async-proxy.h"
#include "common-error.h"
#include "common-message.h"
#include "if-timer-mgr.h"
#include "common-define.h"
#include "protocol.h"

using namespace jukey::util;
using namespace jukey::com;

namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqAsyncProxy::MqAsyncProxy(base::IComFactory* factory, 
	com::IAmqpClient* amqp_client, util::IThread* thread, uint32_t timeout)
	: AsyncProxyBase(factory, thread, timeout)
	, m_amqp_client(amqp_client)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqDefer& MqAsyncProxy::SendMqMsg(const std::string& exchange,
	const std::string& routing_key, const com::Buffer& mq_buf,
	const com::Buffer& sig_buf, uint32_t seq, uint32_t msg)
{
	return SendMqMsg(exchange, routing_key, mq_buf, sig_buf, seq, msg, 0);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqDefer& MqAsyncProxy::SendMqMsg(const std::string& exchange,
	const std::string& routing_key, const com::Buffer& mq_buf,
	const com::Buffer& sig_buf, uint32_t seq, uint32_t msg, uint32_t usr)
{
	MqDeferSP defer(new MqDefer());

	if (ERR_CODE_OK != m_amqp_client->Publish(exchange, routing_key, sig_buf,
		mq_buf)) {
		MakeAsyncError("Send mq message failed", defer);
		UTIL_ERR("Send mq message failed");
		return *(defer.get());
	}

	if (!SaveDefer(seq, msg, usr, defer)) {
		UTIL_ERR("Save async defer failed, seq:{}, msg:{}, usr:{}", seq, msg, usr);
		return *(defer.get());
	}

	StartAsyncTimer(seq, msg, usr, defer);

	return *(defer.get());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MqAsyncProxy::OnMqMsg(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	DeferSP defer = GetDefer(sig_hdr->seq, sig_hdr->mt, sig_hdr->usr);
	if (!defer) {
		UTIL_ERR("Get defer failed, seq:{}, msg:{}, usr:{}", sig_hdr->seq, 
			(uint32_t)sig_hdr->mt, sig_hdr->usr);
		return false;
	}

	MqDeferSP mq_defer = std::dynamic_pointer_cast<MqDefer>(defer);
	if (mq_defer) {
		mq_defer->ReportResponse(mq_buf, sig_buf);
		return true;
	}
	else {
		UTIL_ERR("dynamic pointer cast failed");
		return false;
	}
}

}