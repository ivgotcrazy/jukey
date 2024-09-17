// test-session-client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <thread>

#include "session-client.h"
#include "net-message.h"
#include "common/util-net.h"
#include "common/util-common.h"
#include "common/util-time.h"
#include "event/common-event.h"

using namespace jukey::base;
using namespace jukey::util;

bool g_session_closed = false;

CommonEvent g_send_event;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SendDataFunc(ISessionMgr* mgr, SessionId sid, const ClientParam& param)
{
	uint32_t sleep_interval = 0; // ms 

  uint32_t loop = (param.packet_rate > 0) 
    ? param.packet_rate : (param.bitrate / param.packet_size);

  // Make send smoothly
  if (loop < 100) {
    sleep_interval = 1000;
  }
  else if (loop < 1000) {
    loop = loop / 10;
    sleep_interval = 100;
  }
  else {
    loop = loop / 100;
    sleep_interval = 10;
  }

	Buffer buf(param.packet_size);
	buf.data_len = param.packet_size;
	memset(buf.data.get(), 0, buf.data_len);

	uint64_t total_send = 0;
	for (uint32_t i = 0; i < param.packet_count; ) {
    uint64_t t1 = jukey::util::Now();
		for (uint32_t j = 0; j < loop; j++) {
      if (g_session_closed) {
        printf("Session closed!\n");
        return;
      }

      ErrCode result = mgr->SendData(sid, buf);
      if (result == ERR_CODE_FAILED) { // FIXME: ERR_CODE_SEND_PENDING
        printf("!!! Pending\n");
        g_send_event.Wait(WAIT_INFINITE);
      }
      else if (result != ERR_CODE_OK) {
				printf("Send udp data failed!\n");
			}
			total_send += param.packet_size;
		} 
		i += loop;
    uint64_t t2 = jukey::util::Now();

    if ((t2 - t1) < sleep_interval * 1000) {
      jukey::util::Sleep((sleep_interval * 1000) - (t2 - t1));
    }

		if (i % 10000 == 0) {
			printf("<<< Send packet count:%d, packet size: %lld\n", i, total_send);
		}
	}

	printf("<<< Send packet count:%d, packet size: %lld\n", 
    param.packet_count, total_send);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionClient::SessionClient() : jukey::util::CommonThread("session client", true) 
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionClient::Init(const ClientParam& param)
{
	std::optional<Address> result = ParseAddress(param.addr);
	if (result.has_value()) {
		m_addr = result.value();
	}
	else {
		std::cout << "Invalid address: " << param.addr << std::endl;
		return false;
	}

	m_client_param = param;

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

	SessionMgrParam mgr_param;
	mgr_param.thread_count = 4;
	mgr_param.ka_interval = 3;

	if (ERR_CODE_OK != m_sess_mgr->Init(mgr_param)) {
		std::cout << "Init session manager failed!" << std::endl;
		return false;
	}

	m_sess_mgr->SetLogLevel(m_client_param.log_level);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionClient::Start()
{
	StartThread();

	CreateParam param;
	param.remote_addr = m_addr;
	param.ka_interval = 3; // second
	param.service_type = ServiceType::INVALID;
	param.session_type = m_client_param.reliable ? SessionType::RELIABLE: SessionType::UNRELIABLE;
	param.thread = this;
	param.fec_type = m_client_param.fec ? FecType::LUIGI : FecType::NONE;

	SessionId sid = m_sess_mgr->CreateSession(param);
	if (sid == INVALID_SESSION_ID) {
		printf("Create session failed!\n");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionClient::OnThreadMsg(const jukey::com::CommonMsg& msg)
{
	switch (msg.msg_type) {
	case NET_MSG_SESSION_CREATE_RESULT:
		OnSessionCreateResult(msg);
		break;
	case NET_MSG_SESSION_CLOSED:
		OnSessionClosed();
		break;
	case NET_MSG_SESSION_RESUME:
		printf("### Resume\n");
		g_send_event.Trigger();
		break;
	default:
		printf("Unknown message type:%d\n", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionClient::OnSessionCreateResult(const jukey::com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(SessionCreateResultMsg);
	if (data->result) {
		printf("Session create success\n");
		if (m_client_param.packet_count > 0) {
			std::thread t(SendDataFunc, m_sess_mgr, data->lsid, m_client_param);
			t.detach();
		}
	}
	else {
		printf("Session create failed!\n");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionClient::OnSessionClosed()
{
	g_session_closed = true;
}
