#include "user-msg-builder.h"
#include "user.pb.h"
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
	com::Buffer BuildUserLoginReq(const UserLoginReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::UserLoginReq req;
	req.set_app_id(req_param.app_id);
	req.set_client_id(req_param.client_id);
	req.set_register_id(req_param.register_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_token(req_param.token);

	UTIL_INF("Build user login request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_USER_LOGIN_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildUserLoginRsp(const UserLoginRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::UserLoginRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_client_id(rsp_param.client_id);
	rsp.set_register_id(rsp_param.register_id);
	rsp.set_login_id(rsp_param.login_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_result((uint32_t)rsp_param.result);
	rsp.set_msg(rsp_param.msg);

	UTIL_INF("Build user login response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_USER_LOGIN_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildUserLogoutReq(const UserLogoutReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::UserLogoutReq req;
	req.set_app_id(req_param.app_id);
	req.set_client_id(req_param.client_id);
	req.set_register_id(req_param.register_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_login_id(req_param.login_id);
	req.set_token(req_param.token);

	UTIL_INF("Build user logout request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_USER_LOGOUT_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildUserLogoutRsp(const UserLogoutRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::UserLogoutRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_client_id(rsp_param.client_id);
	rsp.set_register_id(rsp_param.register_id);
	rsp.set_login_id(rsp_param.login_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_result((uint32_t)rsp_param.result);
	rsp.set_msg(rsp_param.msg);

	UTIL_INF("Build user logout response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_USER_LOGOUT_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildUserOfflineNotify(const UserOfflineNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::UserOfflineNotify notify;
	notify.set_app_id(notify_param.app_id);
	notify.set_client_id(notify_param.client_id);
	notify.set_register_id(notify_param.register_id);
	notify.set_user_id(notify_param.user_id);
	notify.set_user_type(notify_param.user_type);
	notify.set_login_id(notify_param.login_id);

	UTIL_INF("Build user offline notify:{}", PbMsgToJson(notify));

	com::Buffer buf((uint32_t)(notify.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_USER_OFFLINE_NOTIFY, buf, notify, hdr_param);

	return buf;
}

}