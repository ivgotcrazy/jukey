#pragma once

#include <optional>
#include <map>

#include "if-service.h"
#include "com-factory.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"
#include "if-amqp-client.h"
#include "async/mq-async-proxy.h"
#include "service-type.h"
#include "if-reporter.h"
#include "config-parser.h"
#include "group-common.h"
#include "msg-sender.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class GroupService 
	: public IService
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
	, public com::IAmqpHandler
{
public:
	GroupService(base::IComFactory* factory, const char* owner);

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
	bool DoInitServiceMq();
	bool DoInitNotifyMq();
	bool DoInitReport();

	void OnMqMsg(const com::CommonMsg& msg);

	void OnServicePingMsg(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);
	void OnJoinGroupReq(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);
	void OnLeaveGroupReq(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);
	void OnPublishMediaReq(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);
	void OnUnpublishMediaReq(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);
	void OnUserOfflineNotify(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);

	UserEntry* FindUserEntry(uint32_t app, uint32_t group, uint32_t user);

	std::optional<GroupEntry> GetGroupEntry(uint32_t app_id, uint32_t group_id);
	UserEntry ParseUserEntry(const prot::JoinGroupReq& req);

	com::ErrCode ProcessJoinGroup(const prot::JoinGroupReq& req);
	com::ErrCode ProcessLeaveGroup(const prot::LeaveGroupReq& req);

private:
	base::IComFactory* m_factory = nullptr;
	com::IAmqpClient* m_amqp_client = nullptr;
	util::MqAsyncProxySP m_mq_async_proxy;
	uint32_t m_cur_seq = 0;
	GroupServiceConfig m_config;
	com::IReporter* m_reporter = nullptr;

	// {AppID:AppEntry}
	std::map<uint32_t, AppEntry> m_app_groups;

	MsgSenderUP m_msg_sender;
};

}