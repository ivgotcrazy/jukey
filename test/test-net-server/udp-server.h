// test-session-server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "if-server.h"
#include "if-udp-mgr.h"
#include "common/util-net.h"
#include "if-timer-mgr.h"

using namespace jukey::net;
using namespace jukey::com;


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
class UdpServer 
	: public IUdpHandler
	, public IServer
	, std::enable_shared_from_this<UdpServer>
{
public:
  // IUdpHandler
  virtual void OnRecvUdpData(const Endpoint& lep, 
    const Endpoint& rep,
    SocketId sock, 
    Buffer buf) override;
  virtual void OnSocketClosed(const Endpoint& lep, 
    const Endpoint& rep,
    SocketId sock) override;

  // IServer
  virtual bool Init(const std::string& addr, uint32_t log_level) override;
  virtual bool Start() override;

  void OnTimeout();

private:
	IUdpMgr*   m_udp_mgr = nullptr;
	ITimerMgr* m_timer_mgr = nullptr;
	Address    m_addr;
	Socket     m_server_socket = 0;
	uint64_t   m_pkt_count = 0;
	uint64_t   m_recv_size = 0;
	TimerId    m_timer_id = INVALID_TIMER_ID;
};