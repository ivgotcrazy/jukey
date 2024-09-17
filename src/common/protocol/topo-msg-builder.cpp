#include "topo-msg-builder.h"
#include "topo.pb.h"
#include "protocol.h"
#include "sig-msg-builder.h"
#include "log/util-log.h"
#include "common/util-pb.h"


using namespace jukey::util;

namespace jukey::prot::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildPingMsg(uint32_t seq,
	const std::string& service_name,
	uint32_t service_type,
	const std::string& instance_id)
{
	jukey::prot::Ping msg;
	msg.set_service_name(service_name);
	msg.set_service_type(service_type);
	msg.set_instance_id(instance_id);

	UTIL_DBG("Build service ping:{}", PbMsgToJson(msg));

	com::SigHdrParam hdr_param;
	hdr_param.seq = seq;

	com::Buffer buf((uint32_t)(msg.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_SERVICE_PING, buf, msg, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildPongMsg(uint32_t seq,
	const std::string& service_name,
	uint32_t service_type,
	const std::string& instance_id)
{
	jukey::prot::Pong msg;
	msg.set_service_name(service_name);
	msg.set_service_type(service_type);
	msg.set_instance_id(instance_id);

	UTIL_DBG("Build service pong:{}", PbMsgToJson(msg));

	com::SigHdrParam hdr_param;
	hdr_param.seq = seq;

	com::Buffer buf((uint32_t)(msg.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_SERVICE_PONG, buf, msg, hdr_param);

	return buf;
}

}