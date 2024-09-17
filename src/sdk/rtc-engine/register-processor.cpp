#include "register-processor.h"
#include "log.h"
#include "rtc-engine-impl.h"
#include "terminal-msg-builder.h"
#include "protoc/terminal.pb.h"
#include "common/util-pb.h"
#include "msg-builder.h"


using namespace jukey::com;
using namespace jukey::prot;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RegisterProcessor::RegisterProcessor(RtcEngineImpl* engine) : m_rtc_engine(engine)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RegisterProcessor::Register()
{
	com::Buffer buf = G(m_msg_builder)->BuildClientRegReq(++G(m_cur_seq));

	G(m_async_proxy)->SendSessionMsg(G(m_proxy_session), buf, G(m_cur_seq),
		MSG_CLIENT_REGISTER_RSP)
		.OnResponse([this](net::SessionId sid, const com::Buffer& buf) {
			OnRegisterRsp(buf);
		})
		.OnTimeout([this]() {
			OnRegisterTimeout();
		})
		.OnError([this](const std::string& err) {
			OnRegisterError(ERR_CODE_FAILED);
		});

	LOG_INF("Send register request");

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RegisterProcessor::Unregister()
{
	std::lock_guard<std::recursive_mutex> lock(G(m_mutex));

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RegisterProcessor::OnRegisterRsp(const com::Buffer& buf)
{
	prot::RegisterRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse register response failed, len:{}", buf.data_len);
		G(m_init_promise).set_value(false);
		return;
	}

	LOG_INF("Received register response:{}", util::PbMsgToJson(rsp));

	if (rsp.result() == (uint32_t)ERR_CODE_OK) {
		G(m_engine_state) = RtcEngineState::INITED;
		G(m_engine_data.register_id) = rsp.register_id();
		G(m_init_promise).set_value(true);
		LOG_INF("Register success");
	}
	else {
		LOG_ERR("Register failed, result:{}", rsp.result());
		G(m_init_promise).set_value(false);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RegisterProcessor::OnRegisterTimeout()
{
	LOG_ERR("Receive register response timeout!");
	G(m_init_promise).set_value(false);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RegisterProcessor::OnRegisterError(com::ErrCode ec)
{
	LOG_ERR("Register error:{}!", ec);
	G(m_init_promise).set_value(false);
}

}