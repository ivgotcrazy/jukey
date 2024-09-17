#pragma once

#include <string>

#include "common-struct.h"

namespace jukey::prot::util
{

////////////////////////////////////////////////////////////////////////////////

struct LoginSendChannelReqParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	com::MediaStream stream;
	std::string token;
};

com::Buffer BuildLoginSendChannelReq(
	const LoginSendChannelReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct LoginSendChannelRspParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	com::MediaStream stream;
	uint32_t channel_id = 0;
	com::ErrCode result = com::ErrCode::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildLoginSendChannelRsp(
	const LoginSendChannelRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct LogoutSendChannelReqParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t channe_id = 0;
	com::MediaStream stream;
	std::string token;
};

com::Buffer BuildLogoutSendChannelReq(
	const LogoutSendChannelReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct LogoutSendChannelRspParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	com::MediaStream stream;
	uint32_t channel_id = 0;
	com::ErrCode result = com::ErrCode::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildLogoutSendChannelRsp(
	const LogoutSendChannelRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct LoginRecvChannelReqParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	com::MediaStream stream;
	std::string token;
};

com::Buffer BuildLoginRecvChannelReq(
	const LoginRecvChannelReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct LoginRecvChannelRspParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t channel_id = 0;
	com::MediaStream stream;
	com::ErrCode result = com::ErrCode::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildLoginRecvChannelRsp(
	const LoginRecvChannelRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct LogoutRecvChannelReqParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t channel_id = 0;
	com::MediaStream stream;
	std::string token;
};

com::Buffer BuildLogoutRecvChannelReq(
	const LogoutRecvChannelReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct LogoutRecvChannelRspParam
{
	uint32_t app_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t channel_id = 0;
	com::MediaStream stream;
	com::ErrCode result = com::ErrCode::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildLogoutRecvChannelRsp(
	const LogoutRecvChannelRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct StartSendStreamNotifyParam
{
	uint32_t channel_id = 0;
	com::MediaStream stream;
};

com::Buffer BuildStartSendStreamNotify(
	const StartSendStreamNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct StartSendStreamAckParam
{
	uint32_t channel_id = 0;
	com::MediaStream stream;
	uint32_t result = 0;
	std::string msg;
};

com::Buffer BuildStartSendStreamAck(
	const StartSendStreamAckParam& ack_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct NegotiateReqParam
{
	uint32_t channel_id = 0;
	com::MediaStream stream;
	std::vector<std::string> caps;
};

com::Buffer BuildNegotiateReq(
	const NegotiateReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct NegotiateRspParam
{
	uint32_t channel_id = 0;
	com::MediaStream stream;
	std::string cap;
	uint32_t result = 0;
	std::string msg;
};

com::Buffer BuildNegotiateRsp(
	const NegotiateRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

}