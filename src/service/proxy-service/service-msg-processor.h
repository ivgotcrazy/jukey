#pragma once

#include "route-entry-mgr.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class ServiceMsgProcessor
{
public:
	void Init(net::ISessionMgr* session_mgr, RouteEntryMgrSP entry_mgr);

	void Process(const net::SessionDataMsgSP& msg);

private:
	void OnClientRegRsp(const net::SessionDataMsgSP& msg);
	void OnUserLoginRsp(const net::SessionDataMsgSP& msg);
	void OnJoinGroupRsp(const net::SessionDataMsgSP& msg);

	void SendMsgToUser(const com::Buffer& msg);
	void SendUnicastMsgToClient(const com::Buffer& msg);
	void SendUnicastMsgToUser(const com::Buffer& msg);
	void SendMulticastMsgToUsers(const com::Buffer& msg);

private:
	net::ISessionMgr* m_sess_mgr = nullptr;
	RouteEntryMgrSP m_route_entry_mgr;
};

}