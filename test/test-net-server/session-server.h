// test-session-server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <iostream>

#include "if-server.h"
#include "if-session-mgr.h"
#include "thread/common-thread.h"
#include "common/util-net.h"

using namespace jukey::net;
using namespace jukey::util;
using namespace jukey::com;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
class SessionServer : public CommonThread, public IServer
{
public:
	SessionServer();

	// IServer
	virtual bool Init(const std::string& addr, uint32_t log_level) override;
	virtual bool Start() override;

	// CommonThread
	virtual void OnThreadMsg(const jukey::com::CommonMsg& msg);

private:
	ISessionMgr* m_sess_mgr = nullptr;
	Address m_addr;
	uint64_t m_recv_data_len = 0;
	uint64_t m_recv_data_count = 0;
};
