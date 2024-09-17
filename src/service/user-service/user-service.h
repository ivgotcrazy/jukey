#pragma once

#include <optional>
#include <memory>
#include <map>

#include "if-service.h"
#include "com-factory.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"
#include "if-amqp-client.h"
#include "service-type.h"
#include "if-reporter.h"
#include "user-common.h"
#include "msg-sender.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class UserService 
	: public IService
	, public base::ProxyUnknown
  , public base::ComObjTracer
	, public util::CommonThread
	, public com::IAmqpHandler
{
public:
	UserService(base::IComFactory* factory, const char* owner);

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IService
	virtual bool Init(net::ISessionMgr* mgr, const std::string& cfg_file) override;
	virtual bool Start() override;
	virtual void Stop() override;
	virtual bool Reload() override;

	// CommonThread
	virtual void OnThreadMsg(const com::CommonMsg& msg) override;

	// IAmqpHandler
	virtual void OnRecvMqMsg(const std::string& queue, 
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf) override;

private:
	bool DoInitMq();
	bool DoInitServiceMq();
	bool DoInitNotifyMq();
	bool DoInitReport();
	
	void OnMqMsg(const com::CommonMsg& msg);

	void OnUserLoginReq(const com::Buffer& mq_buf, const com::Buffer& sig_buf);
	void OnUserLogoutReq(const com::Buffer& mq_buf, const com::Buffer& sig_buf);
  void OnServicePingMsg(const com::Buffer& mq_buf, const com::Buffer& sig_buf);
	void OnClientOfflineNotify(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);

private:
	base::IComFactory* m_factory = nullptr;
	com::IReporter* m_reporter = nullptr;
	com::IAmqpClient* m_amqp_client = nullptr;

	UserServiceConfig m_config;

	uint32_t m_cur_seq = 0;
	uint32_t m_login_id = 0;

	// key: user ID, not support user type
	std::map<uint32_t, UserEntrySP> m_users;

	MsgSenderUP m_msg_sender;
};

}