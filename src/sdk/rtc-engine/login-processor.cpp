#include "login-processor.h"
#include "log.h"
#include "rtc-engine-impl.h"
#include "user-msg-builder.h"
#include "protoc/user.pb.h"
#include "common/util-pb.h"
#include "msg-builder.h"


using namespace jukey::com;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
LoginProcessor::LoginProcessor(RtcEngineImpl* engine) : m_rtc_engine(engine)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode LoginProcessor::Login(uint32_t user_id)
{
	LOG_INF("Login, user:{}", user_id);

	if (user_id == 0) {
		LOG_ERR("Invalid user ID!");
		return ERR_CODE_INVALID_PARAM;
	}

	std::lock_guard<std::recursive_mutex> lock(G(m_mutex));

	if (G(m_engine_state) != RtcEngineState::INITED) {
		LOG_ERR("Invalid state:{} to login!", G(m_engine_state));
		return ERR_CODE_FAILED;
	}

	Buffer buf = G(m_msg_builder)->BuildUserLoginReq(user_id, ++G(m_cur_seq));

	G(m_async_proxy)->SendSessionMsg(G(m_proxy_session), buf, G(m_cur_seq),
		prot::MSG_USER_LOGIN_RSP)
		.OnResponse([this](net::SessionId sid, const com::Buffer& buf) {
			OnUserLoginRsp(buf);
		})
		.OnTimeout([this]() {
			OnUserLoginTimeout();
		})
		.OnError([this](const std::string& err) {
			OnUserLoginError(ERR_CODE_FAILED);
		});

	LOG_INF("Send login request");

	G(m_engine_data.user_id) = user_id;
	G(m_engine_state) = RtcEngineState::LOGINING;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LoginProcessor::OnUserLoginRsp(const com::Buffer& buf)
{
	prot::UserLoginRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse login response failed!");
		G(m_engine_state) = RtcEngineState::INITED;
		G(m_engine_param).handler->OnLoginResult(G(m_engine_data.user_id),
			ERR_CODE_FAILED);
		return;
	}

	LOG_INF("Received user login response:{}", util::PbMsgToJson(rsp));

	if (rsp.result() == (uint32_t)ERR_CODE_OK) {
		LOG_INF("Login success");
		G(m_engine_data.login_id) = rsp.login_id();
		G(m_engine_state) = RtcEngineState::LOGINED;
		G(m_engine_param.handler)->OnLoginResult(G(m_engine_data.user_id),
			ERR_CODE_OK);
	}
	else {
		LOG_INF("Login failed, result:{}, msg:{}", rsp.result(), rsp.msg());
		G(m_engine_state) = RtcEngineState::INITED;
		G(m_engine_param).handler->OnLoginResult(G(m_engine_data.user_id),
			ERR_CODE_FAILED);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LoginProcessor::OnUserLoginTimeout()
{
	LOG_ERR("Receive user login response timeout!");
	G(m_engine_state) = RtcEngineState::INITED;
	G(m_engine_param).handler->OnLoginResult(G(m_engine_data.user_id),
		ERR_CODE_FAILED);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LoginProcessor::OnUserLoginError(com::ErrCode ec)
{
	LOG_ERR("Send login request failed!");
	G(m_engine_state) = RtcEngineState::INITED;
	G(m_engine_param).handler->OnLoginResult(G(m_engine_data.user_id),
		ERR_CODE_FAILED);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode LoginProcessor::Logout()
{
	LOG_INF("Logout");

	std::lock_guard<std::recursive_mutex> lock(G(m_mutex));

	if (G(m_engine_state) != RtcEngineState::LOGINED) {
		LOG_ERR("Invalid state:{} to logout!", G(m_engine_state));
		return ERR_CODE_FAILED;
	}

	Buffer buf = G(m_msg_builder)->BuildUserLogoutReq(++G(m_cur_seq));

	G(m_async_proxy)->SendSessionMsg(G(m_proxy_session), buf, G(m_cur_seq),
		prot::MSG_USER_LOGOUT_RSP)
		.OnResponse([this](net::SessionId sid, const com::Buffer& buf) {
			OnUserLogoutRsp(buf);
		})
		.OnTimeout([this]() {
			OnUserLogoutTimeout();
		})
		.OnError([this](const std::string& err) {
			OnUserLogoutError(ERR_CODE_FAILED);
		});

	LOG_INF("Send logout request");

	G(m_engine_data.login_id) = 0;
	G(m_engine_data.user_id) = 0;

	G(m_engine_state) = RtcEngineState::LOGOUTING;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LoginProcessor::OnUserLogoutRsp(const com::Buffer& buf)
{
	G(m_engine_state) = RtcEngineState::INITED;

	prot::UserLogoutRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse logout response failed!");
		
		G(m_engine_param).handler->OnLogoutResult(G(m_engine_data.user_id),
			ERR_CODE_FAILED);
	}
	else {
		LOG_INF("Received user logout response:{}", util::PbMsgToJson(rsp));

		if (rsp.result() == (uint32_t)ERR_CODE_OK) {
			LOG_INF("Logout success");
			
			G(m_engine_param.handler)->OnLogoutResult(G(m_engine_data.user_id),
				ERR_CODE_OK);
		}
		else {
			LOG_INF("Login failed!");
			G(m_engine_param).handler->OnLogoutResult(G(m_engine_data.user_id),
				ERR_CODE_FAILED);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LoginProcessor::OnUserLogoutTimeout()
{
	LOG_ERR("Receive user login response timeout!");
	G(m_engine_state) = RtcEngineState::INITED;
	G(m_engine_param).handler->OnLogoutResult(G(m_engine_data.user_id),
		ERR_CODE_TIMEOUT);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LoginProcessor::OnUserLogoutError(com::ErrCode ec)
{
	LOG_ERR("Send login request failed!");
	G(m_engine_state) = RtcEngineState::INITED;
	G(m_engine_param).handler->OnLogoutResult(G(m_engine_data.user_id),
		ERR_CODE_FAILED);
}

}