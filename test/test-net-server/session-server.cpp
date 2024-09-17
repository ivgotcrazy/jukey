// test-session-server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <iostream>

#include "session-server.h"
#include "com-factory.h"
#include "net-message.h"
#include "common/util-net.h"
#include "common/util-time.h"

using namespace jukey::util;
using namespace jukey::base;
using namespace std::placeholders;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionServer::SessionServer() : CommonThread("session server", true) 
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionServer::Init(const std::string& addr, uint32_t log_level)
{
	std::optional<Address> result = ParseAddress(addr);
	if (result.has_value()) {
		m_addr = result.value();
	}
	else {
		std::cout << "Invalid address: " << addr << std::endl;
		return false;
	}

	IComFactory* factory = GetComFactory();
	if (!factory) {
		std::cout << "Get component factory failed!" << std::endl;
		return false;
	}

	if (!factory->Init("./")) {
		std::cout << "Init component factory failed!" << std::endl;
		return false;
	}

	m_sess_mgr = (ISessionMgr*)GetComFactory()->QueryInterface(CID_SESSION_MGR,
		IID_SESSION_MGR, "test");
	if (!m_sess_mgr) {
		std::cout << "Create session manager failed!" << std::endl;
		return false;
	}

	SessionMgrParam param;
	param.thread_count = 4;
	param.ka_interval = 5;

	if (ERR_CODE_OK != m_sess_mgr->Init(param)) {
		std::cout << "Init session manager failed!" << std::endl;
		return false;
	}

	m_sess_mgr->SetLogLevel(log_level);

	return true;
}
	
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionServer::Start()
{
	ListenParam param;
	param.listen_addr = m_addr;
	param.listen_srv = ServiceType::INVALID;
	param.thread = this;
	ListenId listen_id = m_sess_mgr->AddListen(param);
	if (listen_id == INVALID_LISTEN_ID) {
		printf("Add listen failed!\n");
		return false;
	}

	StartThread();

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionServer::OnThreadMsg(const jukey::com::CommonMsg& msg)
{
	switch (msg.msg_type) {
	case jukey::net::NET_MSG_SESSION_DATA:
	{
		PCAST_COMMON_MSG_DATA(SessionDataMsg);
		m_recv_data_len += data->buf.data_len;
		m_recv_data_count++;
		break;
	}
	case jukey::net::NET_MSG_SESSION_CLOSED:
	{
		printf(">>> Receive packet count: %lld, packet size: %lld\n", 
			m_recv_data_count, m_recv_data_len);
		m_recv_data_len = 0;
		m_recv_data_count = 0;
		break;
	}
	default:
		printf("msg type: %d\n", msg.msg_type);
	}
}
