#include "session-async-proxy.h"
#include "common-error.h"
#include "common-message.h"
#include "if-timer-mgr.h"
#include "common-define.h"
#include "protocol.h"
#include "log/util-log.h"

using namespace jukey::net;
using namespace jukey::prot;

namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionAsyncProxy::SessionAsyncProxy(base::IComFactory* factory,
	net::ISessionMgr* mgr, util::IThread* thread, uint32_t timeout)
	: AsyncProxyBase(factory, thread, timeout)
	, m_sess_mgr(mgr)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionDefer& SessionAsyncProxy::SendSessionMsg(net::SessionId sid,
	const com::Buffer& buf, uint32_t seq, uint32_t msg)
{
	SessionDeferSP defer(new SessionDefer());

	com::ErrCode ec = m_sess_mgr->SendData(sid, buf);
	if (ec != com::ERR_CODE_OK) {
		UTIL_ERR("Send session msg failed, seq:{}, msg:{}", seq, msg);
		MakeAsyncError("send session msg failed", defer);
		return *(defer.get());
	}

	if (!SaveDefer(seq, msg, 0, defer)) {
		UTIL_ERR("Save async defer failed");
		return *(defer.get());
	}

	StartAsyncTimer(seq, msg, 0, defer);

	return *(defer.get());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionAsyncProxy::OnSessionMsg(net::SessionId sid, const com::Buffer& buf)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(buf);

	DeferSP defer = GetDefer(sig_hdr->seq, sig_hdr->mt, 0);
	if (!defer) {
		UTIL_ERR("Get defer failed, sid:{}, seq:{}, msg:{}", sid, sig_hdr->seq, 
			(uint32_t)sig_hdr->mt);
		return false;
	}

	auto sdefer = std::dynamic_pointer_cast<SessionDefer>(defer);
	if (sdefer) {
		sdefer->ReportResponse(sid, buf);
		return true;
	}
	else {
		UTIL_ERR("dynmaic pointer cast failed");
		return false;
	}
}

}