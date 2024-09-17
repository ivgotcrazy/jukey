#pragma once

#include "common-struct.h"
#include "service-type.h"
#include "protoc/terminal.pb.h"
#include "config-parser.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class MsgBuilder
{
public:
	MsgBuilder(const TerminalServiceConfig& config);

	MqMsgPair BuildClientOfflineRspPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::ClientOfflineReq& req);

	MqMsgPair BuildClientOfflineNotifyPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::ClientOfflineReq& req,
		uint32_t register_id);

	MqMsgPair BuildRegRsp(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::RegisterReq& req,
		uint32_t register_id,
		com::ErrCode result,
		const std::string& msg);

	MqMsgPair BuildUnregRsp(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::UnregisterReq& req,
		com::ErrCode result,
		const std::string& msg);

	MqMsgPair BuildServicePong(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);

private:
	TerminalServiceConfig m_config;
};
typedef std::unique_ptr<MsgBuilder> MsgBuilderUP;

}