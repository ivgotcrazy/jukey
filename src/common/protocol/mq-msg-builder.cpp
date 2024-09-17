#include "mq-msg-builder.h"
#include "mq.pb.h"
#include "log/util-log.h"
#include "common/util-pb.h"

using namespace jukey::util;

namespace jukey::prot::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <class MsgType>
void ConstructMqMsg(uint32_t seq, com::Buffer& buf, MsgType& msg, uint32_t mt)
{
	// Write header
	MqMsgHdr* header = (MqMsgHdr*)buf.data.get();
	header->ver = 1;
	header->res = 0;
	header->mt = mt;
	header->len = (uint16_t)msg.ByteSizeLong(); // TODO:
	header->seq = seq;

	// Write body
	msg.SerializeToArray(buf.data.get() + sizeof(MqMsgHdr),
		(int)msg.ByteSizeLong());

	// Set buffer data length
	buf.data_len = (uint32_t)(msg.ByteSizeLong() + sizeof(MqMsgHdr));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildMqMsg(MsgType msg_type, uint32_t seq, uint32_t service_type,
	const std::string& instance_id, const std::string& exchange, 
	const std::string& routing_key, const std::string& user_data, 
	const std::string& trace_data, const std::string& ext_data)
{
	jukey::prot::MqMsg mq_msg;
	mq_msg.set_service_type(service_type);
	mq_msg.set_instance_id(instance_id);
	mq_msg.set_exchange(exchange);
	mq_msg.set_routing_key(routing_key);
	mq_msg.set_user_data(user_data);
	mq_msg.set_trace_data(trace_data);
	mq_msg.set_extend_data(ext_data);

	UTIL_DBG("Build mq message:{}", PbMsgToJson(mq_msg));

	com::Buffer buf((uint32_t)(mq_msg.ByteSizeLong() + sizeof(MqMsgHdr)));

	ConstructMqMsg(seq, buf, mq_msg, msg_type);

	return buf;
}

}