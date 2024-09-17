#pragma once

#include <map>
#include <mutex>

#include "if-service.h"
#include "com-factory.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"
#include "if-amqp-client.h"
#include "if-stream-exchange.h"
#include "if-reporter.h"
#include "config-parser.h"
#include "async/mq-async-proxy.h"
#include "protoc/transport.pb.h"
#include "msg-sender.h"
#include "common/util-stats.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class TransportService 
	: public IService
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
	, public com::IAmqpHandler
	, public txp::IExchangeHandler
{
public:
	TransportService(base::IComFactory* factory, const char* owner);

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

	// IExchangeHandler
	virtual void OnSendChannelMsg(uint32_t channel_id, uint32_t user_id, 
		const com::Buffer& buf) override;
	virtual void OnSendChannelData(uint32_t channel_id, uint32_t user_id,
		uint32_t mt, const com::Buffer& buf) override;

private:
	void OnMqMsg(const com::CommonMsg& msg);
	
	void OnLoginSendChnlReq(net::SessionId sid, const com::Buffer& buf);
	void OnLogoutSendChnlReq(net::SessionId sid, const com::Buffer& buf);
	void OnLoginRecvChnlReq(net::SessionId sid, const com::Buffer& buf);
	void OnLogoutRecvChnlReq(net::SessionId sid, const com::Buffer& buf);
	void OnStartSendStreamAck(net::SessionId sid, const com::Buffer& buf);

	void OnChannelData(net::SessionId sid, const com::Buffer& buf);
	void OnChannelMsg(net::SessionId sid, const com::Buffer& buf);
	
	void OnSessionClosed(const com::CommonMsg& msg);
	void OnSessionCreateResult(const com::CommonMsg& msg);
	void OnSessionData(const com::CommonMsg& msg);
	void OnSessionIncomming(const com::CommonMsg& msg);

	bool DoInitListen();
	bool DoInitMq();
	bool DoInitStreamExchange();
	bool DoInitReport();
	void DoInitStats();

	void GetParentStreamNode(net::SessionId sid,
		uint32_t channel_id,
		const com::Buffer& buf,
		const prot::LoginRecvChannelReq& req);

private:
	base::IComFactory* m_factory = nullptr;
	net::ISessionMgr* m_sess_mgr = nullptr;

	// Report service dependency state
	com::IReporter* m_reporter = nullptr;
	
	// Listen for transport signal
	net::ListenId m_listen_id = INVALID_LISTEN_ID;

	// Communicate with rabbitmq
	com::IAmqpClient* m_amqp_client = nullptr;

	// Stream data exchange
	txp::IStreamExchange* m_stream_exch = nullptr;

	// Allocate signal sequence
	uint32_t m_cur_seq = 0;

	// 1 ...
	uint32_t m_chnl_id = 0;

	struct ChannelEntry
	{
		ChannelEntry(uint32_t cid, bool s, const std::string& sid) 
			: channel_id(cid), send(s), stream_id(sid) {}

		uint32_t channel_id = 0;
		bool send = false;
		std::string stream_id;
	};
	std::map<net::SessionId, ChannelEntry> m_sess_chnl;

	// Service configure
	TransportServiceConfig m_config;

	// Aysnc helper for communicating with rabbitmq
	util::MqAsyncProxySP m_mq_async_proxy;

	MsgSenderUP m_msg_sender;

	util::DataStatsSP m_data_stats;
	util::StatsId m_recv_br_id = INVALID_STATS_ID;
	util::StatsId m_send_br_id = INVALID_STATS_ID;
};

}