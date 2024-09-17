// test-session-client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "if-client.h"
#include "if-udp-mgr.h"
#include "common/util-common.h"

using namespace jukey::net;
using namespace jukey::com;

//==============================================================================
// 
//==============================================================================
class UdpClient : public IUdpHandler, public IClient
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

  // IClient
  virtual bool Init(const ClientParam& param) override;
  virtual bool Start() override;

private:
	IUdpMgr* m_udp_mgr = nullptr;
  ClientParam m_param;
  Address m_addr;
	Socket m_client_socket = 0;
};
