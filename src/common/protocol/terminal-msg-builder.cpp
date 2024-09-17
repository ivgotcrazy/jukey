#include "terminal-msg-builder.h"
#include "terminal.pb.h"
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
com::Buffer BuildClientRegReq(const ClientRegReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::RegisterReq req;
	req.set_app_id(req_param.app_id);
	req.set_client_id(req_param.client_id);
	req.set_client_name(req_param.client_name);
	req.set_client_type(req_param.client_type);
	req.set_secret(req_param.secret);
	req.set_os(req_param.os);
	req.set_device(req_param.device);
	req.set_version(req_param.version);

	UTIL_INF("Build register request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_CLIENT_REGISTER_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildClientRegRsp(const ClientRegRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::RegisterRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_client_id(rsp_param.client_id);
	rsp.set_register_id(rsp_param.register_id);
	rsp.set_result((uint32_t)rsp_param.result);
	rsp.set_msg(rsp_param.msg);

	UTIL_INF("Build register response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_CLIENT_REGISTER_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildClientUnregReq(const ClientUnregReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::UnregisterReq req;
	req.set_app_id(req_param.app_id);
	req.set_client_id(req_param.client_id);
	req.set_register_id(req_param.register_id);
	req.set_secret(req_param.secret);

	UTIL_INF("Build unregister request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_CLIENT_UNREGISTER_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildClientUnregRsp(const ClientUnregRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::UnregisterRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_client_id(rsp_param.client_id);
	rsp.set_register_id(rsp_param.register_id);
	rsp.set_result((uint32_t)rsp_param.result);
	rsp.set_msg(rsp_param.msg);

	UTIL_INF("Build unregister response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_CLIENT_UNREGISTER_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildClientOfflineReq(const ClientOfflineReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::ClientOfflineReq req;
	req.set_app_id(req_param.app_id);
	req.set_client_id(req_param.client_id);
	req.set_instance_id(req_param.instance_id);
	req.set_session_id(req_param.session_id);

	UTIL_INF("Build client offline request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_CLIENT_OFFLINE_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildClientOfflineRsp(const ClientOfflineRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::ClientOfflineRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_client_id(rsp_param.client_id);
	rsp.set_session_id(rsp_param.session_id);
	rsp.set_instance_id(rsp_param.instance_id);

	UTIL_INF("Build client offline response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_CLIENT_OFFLINE_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildClientOfflineNotify(const ClientOfflineNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::ClientOfflineNotify notify;
	notify.set_app_id(notify_param.app_id);
	notify.set_client_id(notify_param.client_id);
	notify.set_register_id(notify_param.register_id);

	UTIL_INF("Build client offline notify:{}", PbMsgToJson(notify));

	com::Buffer buf((uint32_t)(notify.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_CLIENT_OFFLINE_NOTIFY, buf, notify, hdr_param);

	return buf;
}

}