#pragma once

#include <string>

#include "common-struct.h"

namespace jukey::prot::util
{

////////////////////////////////////////////////////////////////////////////////

struct ClientRegReqParam
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint32_t client_type = 0;
	std::string client_name;
	std::string os;
	std::string version;
	std::string device;
	std::string secret;
};

com::Buffer BuildClientRegReq(
  const ClientRegReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct ClientRegRspParam
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint32_t register_id = 0;
	com::ErrCode result = com::ErrCode::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildClientRegRsp(
	const ClientRegRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct ClientUnregReqParam
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint32_t register_id = 0;
	std::string secret;
};

com::Buffer BuildClientUnregReq(
	const ClientUnregReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct ClientUnregRspParam
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint32_t register_id = 0;
	com::ErrCode result = com::ErrCode::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildClientUnregRsp(
	const ClientUnregRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct ClientOfflineReqParam
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint16_t session_id = 0;
	std::string instance_id;
};

com::Buffer BuildClientOfflineReq(
	const ClientOfflineReqParam& req_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct ClientOfflineRspParam
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint32_t session_id = 0;
	std::string instance_id;
};

com::Buffer BuildClientOfflineRsp(
	const ClientOfflineRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct ClientOfflineNotifyParam
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	uint32_t register_id = 0;
};

com::Buffer BuildClientOfflineNotify(
	const ClientOfflineNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param);

}