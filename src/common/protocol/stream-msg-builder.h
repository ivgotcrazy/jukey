#pragma once

#include <string>
#include <vector>

#include "common-struct.h"

namespace jukey::prot::util
{

////////////////////////////////////////////////////////////////////////////////

struct PubStreamReqParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	com::MediaStream stream;
	std::string token;
};

com::Buffer BuildPubStreamReq(
	const PubStreamReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct PubStreamRspParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	com::MediaStream stream;
	com::ErrCode result = com::ErrCode::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildPubStreamRsp(
	const PubStreamRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct UnpubStreamReqParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	com::MediaStream stream;
	std::string token;
};

com::Buffer BuildUnpubStreamReq(
	const UnpubStreamReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct UnpubStreamRspParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	com::MediaStream stream;
	com::ErrCode result = com::ErrCode::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildUnpubStreamRsp(
	const UnpubStreamRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct SubStreamReqParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	com::MediaStream stream;
	std::string token;
};

com::Buffer BuildSubStreamReq(
	const SubStreamReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct SubStreamRspParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	com::MediaStream stream;
	std::string stream_addr;
	com::ErrCode result = com::ErrCode::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildSubStreamRsp(
	const SubStreamRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct UnsubStreamReqParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	com::MediaStream stream;
	std::string token;
};

com::Buffer BuildUnsubStreamReq(
	const UnsubStreamReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct UnsubStreamRspParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	com::MediaStream stream;
	com::ErrCode result = com::ErrCode::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildUnsubStreamRsp(
	const UnsubStreamRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct LoginSendChannelNotifyParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	com::MediaStream stream;
	std::string stream_addr;
};

com::Buffer BuildLoginSendChannelNotify(
	const LoginSendChannelNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct LoginSendChannelAckParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	com::MediaStream stream;
	uint32_t result = 0;
	std::string msg;
};

com::Buffer BuildLoginSendChannelAck(
	const LoginSendChannelAckParam& ack_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct GetParentNodeReqParam
{
	uint32_t service_type = 0;
	std::string instance_id;
	std::string service_addr;
	com::MediaStream stream;
};

com::Buffer BuildGetParentNodeReq(
	const GetParentNodeReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct StreamNodeEntry
{
	uint32_t service_type = 0;
	std::string instance_id;
	std::string service_addr;
};

struct GetParentNodeRspParam
{
	com::MediaStream stream;
	std::vector<StreamNodeEntry> nodes;
};

com::Buffer BuildGetParentNodeRsp(
	const GetParentNodeRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

}