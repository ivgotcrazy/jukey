#include "msg-parser.h"
#include "protocol.h"
#include "log.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RegisterReqTupleOpt ParseRegisterReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);
	if (sig_hdr->e == 0) {
		LOG_ERR("No extend data");
		return std::nullopt;
	}

	if (sig_buf.data_len <= sig_hdr->len) {
		LOG_ERR("Invalid len, buf:{}, hdr:{}", sig_buf.data_len, sig_hdr->len);
		return std::nullopt;
	}

	prot::RegisterReq req;
	if (!req.ParseFromArray(DP(sig_buf) + sizeof(prot::SigMsgHdr), sig_hdr->len)) {
		LOG_ERR("Parse register request failed!");
		return std::nullopt;
	}

	uint32_t ext_len = sig_buf.data_len - sizeof(prot::SigMsgHdr) - sig_hdr->len;
	uint32_t ext_pos = sig_buf.start_pos + sizeof(prot::SigMsgHdr) + sig_hdr->len;

	prot::RegisterReqExtendData extend_data;
	if (!extend_data.ParseFromArray(sig_buf.data.get() + ext_pos, ext_len)) {
		LOG_ERR("Parse extend data failed");
		return std::nullopt;
	}

	// Parse MQ message
	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<RegisterReqTuple>(mq_msg, req, extend_data);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
UnregisterReqPairOpt ParseUnregisterReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::UnregisterReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse unregister request failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<UnregisterReqPair>(mq_msg, req);
}

}