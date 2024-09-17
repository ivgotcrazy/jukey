#pragma once

#include <string>

#include "common-struct.h"

namespace jukey::prot::util
{

////////////////////////////////////////////////////////////////////////////////

struct UserLoginReqParam
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint32_t register_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	std::string token;
};

com::Buffer BuildUserLoginReq(
	const UserLoginReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct UserLoginRspParam
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint32_t register_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t login_id = 0;
	com::ErrCode result = com::ErrCode::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildUserLoginRsp(
	const UserLoginRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct UserLogoutReqParam
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint32_t register_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t login_id = 0;
	std::string token;
};

com::Buffer BuildUserLogoutReq(
	const UserLogoutReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct UserLogoutRspParam
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint32_t register_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t login_id = 0;
	com::ErrCode result = com::ErrCode::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildUserLogoutRsp(
	const UserLogoutRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct UserOfflineNotifyParam
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint32_t register_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t login_id = 0;
};

com::Buffer BuildUserOfflineNotify(
	const UserOfflineNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param);

}