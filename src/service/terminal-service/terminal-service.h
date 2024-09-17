#pragma once

#include <set>
#include <optional>

#include "if-service.h"
#include "com-factory.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"
#include "if-amqp-client.h"
#include "service-type.h"
#include "if-reporter.h"
#include "msg-sender.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class TerminalService 
	: public IService
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
	, public com::IAmqpHandler
{
public:
	TerminalService(base::IComFactory* factory, const char* owner);

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IService
	virtual bool Init(net::ISessionMgr* mgr, 
		const std::string& config_file) override;
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
	bool DoInitReport();

	void OnMqMsg(const com::CommonMsg& msg);

	void OnRegisterReq(const com::Buffer& mq_buf, 
		const com::Buffer& sig_buf);
	void OnUnregisterReq(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);
	void OnServicePingMsg(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);
	void OnClientOfflineReq(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);

private:
	struct TerminalEntry
	{
		uint64_t register_time = 0;
		uint32_t register_id = 0;
		net::SessionId session_id = 0;
		std::string instance_id; // proxy service
		uint32_t client_id;
		uint32_t client_type;
		std::string client_name;
		std::string os;
		std::string version;
		std::string device;
	};

private:
	base::IComFactory* m_factory = nullptr;
	com::IReporter* m_reporter = nullptr;
	com::IAmqpClient* m_amqp_client = nullptr;
	TerminalServiceConfig m_config;
	MsgSenderUP m_msg_sender;

	uint32_t m_next_seq = 1;
	uint32_t m_cur_reg_id = 0;

	// key: register ID
	std::map<uint32_t, TerminalEntry> m_terminals;
};

}