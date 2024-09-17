#pragma once

#include <memory>

#include "if-session-mgr.h"
#include "protoc/transport.pb.h"

namespace jukey::srv
{

class MsgSender
{
public:
	MsgSender(net::ISessionMgr* session_mgr);

	void SendLoginRecvChnlRsp(net::SessionId sid,
		uint32_t channel_id,
		const com::Buffer& buf,
		const prot::LoginRecvChannelReq& req,
		com::ErrCode result,
		const std::string& msg);

	void SendLoginSendChnlRsp(net::SessionId sid,
		uint32_t channel_id,
		const com::Buffer& buf,
		const prot::LoginSendChannelReq& req,
		com::ErrCode result,
		const std::string& msg);

	void SendLogoutSendChnlRsp(net::SessionId sid,
		uint32_t channel_id,
		const com::Buffer& buf,
		const prot::LogoutSendChannelReq& req,
		com::ErrCode result,
		const std::string& msg);

	void SendStartSendStreamNotify(uint32_t seq,
		net::SessionId sid,
		uint32_t channel_id,
		const com::Buffer& buf,
		const prot::LoginSendChannelReq& req);

private:
	net::ISessionMgr* m_sess_mgr = nullptr;
};
typedef std::unique_ptr<MsgSender> MsgSenderUP;

}