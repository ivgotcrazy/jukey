#pragma once

#include <mutex>
#include <vector>
#include <future>

#include "include/if-amqp-client.h"
#include "if-tcp-mgr.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "com-factory.h"
#include "amqpcpp.h"
#include "if-timer-mgr.h"


namespace jukey::com
{

//==============================================================================
// 
//==============================================================================
class AmqpClient 
	: public IAmqpClient
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public net::ITcpHandler
	, public AMQP::ConnectionHandler
{
public:
	AmqpClient(base::IComFactory* factory, const char* owner);
	~AmqpClient();

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IAmqpClient
	virtual com::ErrCode Init(
		const AmqpParam& param) override;
	virtual com::ErrCode DeclareExchange(
		const std::string& name, 
		ExchangeType type) override;
	virtual com::ErrCode RemoveExchange(
		const std::string& name) override;
	virtual com::ErrCode DeclareQueue(
		const std::string& name) override;
	virtual com::ErrCode RemoveQueue(
		const std::string& name) override;
	virtual com::ErrCode BindQueue(
		const std::string& exchange, 
		const std::string& queue, 
		const std::string& bind_key) override;
	virtual com::ErrCode Publish(
		const std::string& exchange,
		const std::string& routing_key,
		const com::Buffer& sig_buf,
		const com::Buffer& mq_buf) override;
	virtual com::ErrCode Publish(
	const std::string& exchange,
	const std::string& routing_key,
	const com::Buffer& buf) override;

	// ITcpHandler
	virtual void OnIncommingConn(
		const com::Endpoint& lep,
		const com::Endpoint& rep,
		net::SocketId sock) override;
	virtual void OnConnectResult(
		const com::Endpoint& lep,
		const com::Endpoint& rep,
		net::SocketId sock,
		bool result) override;
	virtual void OnConnClosed(
		const com::Endpoint& lep,
		const com::Endpoint& rep,
		net::SocketId sock) override;
	virtual void OnRecvTcpData(
		const com::Endpoint& lep,
		const com::Endpoint& rep,
		net::SocketId sock,
		com::Buffer buf) override;

	// ConnectionHandler
	virtual void onProperties(
		AMQP::Connection* connection,
		const AMQP::Table& server,
		AMQP::Table& client) override;
	virtual uint16_t onNegotiate(
		AMQP::Connection* connection,
		uint16_t interval) override;
	virtual void onData(
		AMQP::Connection* connection,
		const char* buffer, 
		size_t size) override;
	virtual void onHeartbeat(
		AMQP::Connection* connection) override;
	virtual void onError(
		AMQP::Connection* connection,
		const char* message) override;
	virtual void onReady(
		AMQP::Connection* connection) override;
	virtual void onClosed(
		AMQP::Connection* connection) override;

	void SendAmqpHeartBeat();

private:
	bool CheckAmqpParam(const AmqpParam& param);
	void StartHeartBeat(uint16_t interval);

private:
	base::IComFactory* m_factory = nullptr;
	net::ITcpMgr* m_tcp_mgr = nullptr;
	AmqpParam m_param;
	AMQP::Connection* m_amqp_conn = nullptr;
	AMQP::Channel* m_amqp_chnl = nullptr;
	std::recursive_mutex m_mutex;
	net::SocketId m_tcp_conn = INVALID_SOCKET_ID;
	std::vector<char> m_recv_buf;
	std::promise<bool> m_conn_promise;
	com::ITimerMgr* m_timer_mgr = nullptr;
	com::TimerId m_amqp_heartbeat_timer = INVALID_TIMER_ID;
};

}