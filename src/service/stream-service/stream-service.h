#pragma once

#include <unordered_map>
#include <vector>
#include <optional>

#include "if-service.h"
#include "com-factory.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"
#include "if-amqp-client.h"
#include "service-type.h"
#include "if-reporter.h"
#include "config-parser.h"
#include "async/mq-async-proxy.h"
#include "stream-common.h"
#include "msg-sender.h"


namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class StreamService 
	: public IService
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
	, public com::IAmqpHandler
{
public:
	StreamService(base::IComFactory* factory, const char* owner);

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

  void OnServicePingMsg(const com::Buffer& mq_buf, const com::Buffer& sig_buf);
	void OnPubStreamReq(const com::Buffer& mq_buf, const com::Buffer& sig_buf);
	void OnSubStreamReq(const com::Buffer& mq_buf, const com::Buffer& sig_buf);
	void OnUnpubStreamReq(const com::Buffer& mq_buf, const com::Buffer& sig_buf);
	void OnUnsubStreamReq(const com::Buffer& mq_buf, const com::Buffer& sig_buf);
	void OnGetParentNodeReq(const com::Buffer& mq_buf, const com::Buffer& sig_buf);

private:
	base::IComFactory* m_factory = nullptr;
	com::IAmqpClient* m_amqp_client = nullptr;
	com::IReporter* m_reporter = nullptr;
	std::string m_service_queue;
	std::unordered_map<std::string, com::MediaStream> m_streams;
	StreamServiceConfig m_config;
	util::MqAsyncProxySP m_mq_async_proxy;
	uint32_t m_cur_seq = 0;
	MsgSenderUP m_msg_sender;
};

}