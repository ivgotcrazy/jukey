#include "msg-builder.h"
#include "protocol.h"
#include "stream-msg-builder.h"
#include "mq-msg-builder.h"
#include "common/util-pb.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// TODO: MQ message and signal message have the same seq
//------------------------------------------------------------------------------
MqMsgPair BuildGetParentNodeReqPair(uint32_t seq,
	const prot::LoginRecvChannelReq& req, 
	const std::string& addr, 
	const std::string& exchange, 
	uint32_t service_type,
	const std::string& instance_id)
{
	prot::util::GetParentNodeReqParam req_param;
	req_param.service_type = service_type;
	req_param.instance_id = instance_id;
	req_param.service_addr = addr;
	req_param.stream = util::ToMediaStream(req.stream());

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req.app_id();
	hdr_param.seq = seq;

	Buffer send_sig_buf = BuildGetParentNodeReq(req_param, hdr_param);

	Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_BETWEEN_SERVICE,
		seq,
		service_type,
		instance_id,
		exchange,
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return MqMsgPair(send_mq_buf, send_sig_buf);
}

}