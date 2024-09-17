#pragma once

#include "if-session-mgr.h"
#include "async-proxy-base.h"

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class SessionAsyncProxy : public AsyncProxyBase
{
public:
	SessionAsyncProxy(base::IComFactory* factory,
		net::ISessionMgr* mgr,
		util::IThread* thread,
		uint32_t timeout);

	SessionDefer& SendSessionMsg(
		net::SessionId sid,
		const com::Buffer& buf,
		uint32_t seq,
		uint32_t msg);

	bool OnSessionMsg(net::SessionId sid, const com::Buffer& buf);

private:
	net::ISessionMgr* m_sess_mgr = nullptr;
};
typedef std::shared_ptr<SessionAsyncProxy> SessionAsyncProxySP;

}