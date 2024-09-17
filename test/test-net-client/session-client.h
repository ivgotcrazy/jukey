// test-session-client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "if-client.h"
#include "if-session-mgr.h"
#include "thread/common-thread.h"
#include "common/util-common.h"

using namespace jukey::net;
using namespace jukey::com;

//==============================================================================
// 
//==============================================================================
class SessionClient : public jukey::util::CommonThread, public IClient
{
public:
  SessionClient();

  // IClient
  virtual bool Init(const ClientParam& param) override;
  virtual bool Start() override;

  // CommonThread
  virtual void OnThreadMsg(const jukey::com::CommonMsg& msg);

  void OnSessionCreateResult(const jukey::com::CommonMsg& msg);
  void OnSessionClosed();

private:
  ISessionMgr* m_sess_mgr = nullptr;
  Address m_addr;
  ClientParam m_client_param;
};
