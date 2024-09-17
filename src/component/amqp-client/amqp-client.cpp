#include "amqp-client.h"
#include "protocol.h"
#include "log.h"


using namespace jukey::util;
using namespace AMQP;


namespace jukey::com
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AmqpClient::AmqpClient(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_AMQP_CLIENT, owner)
	, m_factory(factory)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AmqpClient::~AmqpClient()
{
	if (m_tcp_mgr) {
		m_tcp_mgr->Release();
		m_tcp_mgr = nullptr;
	}

	if (m_amqp_heartbeat_timer != INVALID_TIMER_ID) {
		m_timer_mgr->StopTimer(m_amqp_heartbeat_timer);
		m_timer_mgr->FreeTimer(m_amqp_heartbeat_timer);
		m_amqp_heartbeat_timer = INVALID_TIMER_ID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* AmqpClient::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_AMQP_CLIENT) == 0) {
		return new AmqpClient(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* AmqpClient::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_AMQP_CLIENT)) {
		return static_cast<IAmqpClient*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool AmqpClient::CheckAmqpParam(const AmqpParam& param)
{
	if (!param.handler) {
		LOG_ERR("Invalid handler!");
		return false;
	}

	if (param.host.empty()) {
		LOG_ERR("Invalid host!");
		return false;
	}

	if (param.port == 0) {
		LOG_ERR("Invalid port!");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AmqpClient::Init(const AmqpParam& param)
{
	if (!CheckAmqpParam(param)) {
		return ERR_CODE_INVALID_PARAM;
	}
	m_param = param;

	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		m_timer_mgr = QUERY_TIMER_MGR(m_factory);
		assert(m_timer_mgr);

		m_tcp_mgr = (net::ITcpMgr*)QI(CID_TCP_MGR, IID_TCP_MGR, "amqp client");
		if (!m_tcp_mgr) {
			LOG_ERR("[owner:{}] Create tcp manager failed!", param.owner);
			return ERR_CODE_FAILED;
		}

		if (m_tcp_mgr->Init(this, 1) != ERR_CODE_OK) {
			LOG_ERR("[owner:{}] Init tcp manager failed!", param.owner);
			return ERR_CODE_FAILED;
		}

		if (ERR_CODE_OK != m_tcp_mgr->Connect(Endpoint(param.host, param.port))) {
			LOG_ERR("[owner:{}] Connect {}:{} failed!", param.host, param.port,
				param.owner);
			return ERR_CODE_FAILED;
		}
	}

	return m_conn_promise.get_future().get() ? ERR_CODE_OK : ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AmqpClient::DeclareExchange(const std::string& name, ExchangeType type)
{
	if (!m_amqp_conn || !m_amqp_chnl) {
		LOG_ERR("[owner:{}] Invalid connection or channel!", m_param.owner);
		return ERR_CODE_FAILED;
	}

	std::promise<bool> result;
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		auto SuccessCb = [this, name, &result]() {
			LOG_INF("[owner:{}] Declare exchange:{} success", name, m_param.owner);
			result.set_value(true);
		};

		auto ErrorCb = [this, name, &result](const std::string& msg) {
			LOG_ERR("[owner:{}] Declare exchange:{} failed!", name, m_param.owner);
			result.set_value(false);
		};

		m_amqp_chnl->declareExchange(name, AMQP::ExchangeType(type))
			.onSuccess(SuccessCb)
			.onError(ErrorCb);
	}

	return result.get_future().get() ? ERR_CODE_OK : ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AmqpClient::RemoveExchange(const std::string& name)
{
	std::promise<bool> result;
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		m_amqp_chnl->removeExchange(name)
			.onSuccess([name, &result]() {
			LOG_INF("Remove exchange:{} success", name);
			result.set_value(true);
				})
			.onError([name, &result](const std::string& msg) {
					LOG_ERR("Remove exchange:{} failed, error:{}", name, msg);
					result.set_value(false);
				});
	}

	return result.get_future().get() ? ERR_CODE_OK : ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AmqpClient::DeclareQueue(const std::string& name)
{
	if (!m_amqp_conn || !m_amqp_chnl) {
		LOG_ERR("[owner:{}] Invalid amqp connection or channel!", m_param.owner);
		return ERR_CODE_FAILED;
	}

	std::promise<bool> result;
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		auto SuccessCb = [this, name, &result]() {
			LOG_INF("[owner:{}] Declare queue:{} Success", name, m_param.owner);
			result.set_value(true);
		};

		auto ErrorCb = [this, name, &result](const std::string& msg) {
			LOG_ERR("[owner:{}] Declare queue:{} failed!", name, m_param.owner);
			result.set_value(false);
		};

		m_amqp_chnl->declareQueue(name)
			.onSuccess(SuccessCb)
			.onError(ErrorCb);
	}

	return result.get_future().get() ? ERR_CODE_OK : ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AmqpClient::RemoveQueue(const std::string& name)
{
	std::promise<bool> result;
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		m_amqp_chnl->removeQueue(name)
			.onSuccess([name, &result]() {
			LOG_INF("Remove exchange:{} success", name);
			result.set_value(true);
				})
			.onError([name, &result](const std::string& msg) {
					LOG_ERR("Remove exchange:{} failed, error:{}", name, msg);
					result.set_value(false);
				});
	}

	return result.get_future().get() ? ERR_CODE_OK : ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AmqpClient::BindQueue(const std::string& exchange,
	const std::string& queue, const std::string& bind_key)
{
	LOG_INF("[owner:{}] Bind queue, exchange:{}, queue:{}, bind_key:{}", exchange,
		m_param.owner, queue, bind_key);

	if (!m_amqp_conn || !m_amqp_chnl) {
		LOG_ERR("[owner:{}] Invalid amqp connection or channel!", m_param.owner);
		return ERR_CODE_FAILED;
	}

	std::promise<bool> result;
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		auto BindSuccessCb = [this, exchange, queue, bind_key, &result]() {
			LOG_INF("[owner:{}] Bind queue success", m_param.owner);
			auto startCb = [&result, exchange, queue](const std::string& tag) {
				LOG_INF("Consume success");
				result.set_value(true);
			};

			auto errorCb = [&result, exchange, queue](const char* message) {
				LOG_ERR("Consume failed");
				result.set_value(false);
			};

			auto messageCb = [this, queue](const AMQP::Message& msg,
				uint64_t deliveryTag, bool redelivered) {

					// copy MQ message
					prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)msg.body();
					Buffer mq_buf(msg.body(), mq_hdr->len + sizeof(prot::MqMsgHdr));

					// Copy signal message
					prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)(msg.body() + mq_buf.data_len);
					Buffer sig_buf(msg.body() + mq_buf.data_len, 
						(uint32_t)msg.bodySize() - mq_buf.data_len);

					// Callback
					m_param.handler->OnRecvMqMsg(queue, mq_buf, sig_buf);

					// Ack after processed
					m_amqp_chnl->ack(deliveryTag);
			};

			m_amqp_chnl->consume(queue)
				.onReceived(messageCb)
				.onSuccess(startCb)
				.onError(errorCb);
		};

		auto BindErrorCb = [&result](const std::string& error) {
			LOG_ERR("Bind queue failed");
			result.set_value(false);
		};

		m_amqp_chnl->bindQueue(exchange, queue, bind_key)
			.onSuccess(BindSuccessCb)
			.onError(BindErrorCb);
	}

	return result.get_future().get() ? ERR_CODE_OK : ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AmqpClient::Publish(const std::string& exchange,
	const std::string& routing_key, const com::Buffer& sig_buf,
	const com::Buffer& mq_buf)
{
	LOG_INF("[owner:{}] start publish 1", m_param.owner);

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	LOG_INF("[owner:{}] start publish 2", m_param.owner);

	// New buffer with MQ message data and protocol data
	com::Buffer send_buf(sig_buf.data_len + mq_buf.data_len);

	// Copy MQ data
	memcpy(send_buf.data.get(), mq_buf.data.get(), mq_buf.data_len);

	// Copy protocol data
	memcpy(send_buf.data.get() + mq_buf.data_len, sig_buf.data.get(),
		sig_buf.data_len);

	// Update buffer data length
	send_buf.data_len = sig_buf.data_len + mq_buf.data_len;

	if (!m_amqp_chnl->publish(exchange, routing_key, (const char*)DP(send_buf),
		(size_t)send_buf.data_len)) {
		LOG_ERR("[owner:{}] Publish data failed, exchange:{}, routing key:{}",
			m_param.owner, exchange, routing_key);
		return ERR_CODE_FAILED;
	}

	LOG_INF("[owner:{}] Publish data, exchange:{}, routing key:{}, sig len:{}",
		m_param.owner, exchange, routing_key, sig_buf.data_len);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AmqpClient::Publish(const std::string& exchange,
	const std::string& routing_key, const com::Buffer& buf)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	if (!m_amqp_chnl->publish(exchange, routing_key,
		(const char*)buf.data.get(), (size_t)buf.data_len)) {
		LOG_ERR("[owner:{}] Publish data failed, exchange:{}, routing key:{}",
			m_param.owner, exchange, routing_key);
		return ERR_CODE_FAILED;
	}

	LOG_INF("[owner:{}] Publish data, exchange:{}, routing key:{}, data len:{}",
		m_param.owner, exchange, routing_key, buf.data_len);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AmqpClient::OnIncommingConn(const Endpoint& lep, const Endpoint& rep,
	net::SocketId sock)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AmqpClient::OnConnectResult(const Endpoint& lep, const Endpoint& rep,
	net::SocketId sock, bool result)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	if (!result) {
		m_conn_promise.set_value(false);
	}
	else {
		m_tcp_conn = sock;
		m_amqp_conn = new AMQP::Connection(this,
			AMQP::Login(m_param.user, m_param.pwd), "/");
		m_amqp_chnl = new AMQP::Channel(m_amqp_conn);

		// Wait OnReady to confirm login success
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AmqpClient::OnConnClosed(const Endpoint& lep, const Endpoint& rep,
	net::SocketId sock)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	LOG_WRN("[owner:{}] Connection closed, socket:{}, local:{}, remote:{}",
		m_param.owner, sock, lep.ToStr(), rep.ToStr());

	m_tcp_conn = INVALID_SOCKET_ID;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AmqpClient::OnRecvTcpData(const Endpoint& lep, const Endpoint& rep,
	net::SocketId sock, com::Buffer buf)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	m_recv_buf.insert(m_recv_buf.end(), buf.data.get(), buf.data.get() + buf.data_len);

	auto expected_size = m_amqp_conn->expected();
	while (m_recv_buf.size() >= expected_size) {
		std::vector<char> buf(m_recv_buf.begin(), m_recv_buf.begin() + expected_size);
		uint64_t parsed = m_amqp_conn->parse(buf.data(), buf.size());
		if (parsed != 0) {
			m_recv_buf.erase(m_recv_buf.begin(), m_recv_buf.begin() + parsed);
		}
		expected_size = m_amqp_conn->expected();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AmqpClient::onProperties(Connection* connection, const Table& server,
	Table& client)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// interval is in second
//------------------------------------------------------------------------------
void AmqpClient::StartHeartBeat(uint16_t interval)
{
	if (m_amqp_heartbeat_timer != INVALID_TIMER_ID) {
		m_timer_mgr->StopTimer(m_amqp_heartbeat_timer);
		m_amqp_heartbeat_timer = INVALID_TIMER_ID;
	}
	
	com::TimerParam timer_param;
	timer_param.timeout = interval * 1000;
	timer_param.timer_type = TimerType::TIMER_TYPE_LOOP;
	timer_param.timer_name = "amqp client";
	timer_param.user_data = 0;
	timer_param.timer_func = [this](int64_t) {
		SendAmqpHeartBeat();
	};

	m_amqp_heartbeat_timer = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(m_amqp_heartbeat_timer);
}

//------------------------------------------------------------------------------
// interval is in second
//------------------------------------------------------------------------------
uint16_t AmqpClient::onNegotiate(Connection* connection, uint16_t interval)
{
	LOG_INF("{}, interval:{}", __FUNCTION__, interval);

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	if (interval < 60) {
		LOG_INF("[owner:{}] Negotiate interval:{} is set to 60", m_param.owner,
			interval);
		interval = 60;
	}

	StartHeartBeat(interval);

	return interval;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AmqpClient::onData(Connection* connection, const char* buffer, size_t size)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	if (m_tcp_conn == INVALID_SOCKET_ID) {
		LOG_ERR("Disconnected!");
		return;
	}

	if (ERR_CODE_OK != m_tcp_mgr->SendData(m_tcp_conn,
		com::Buffer(buffer, (uint32_t)size))) {
		LOG_ERR("Send data failed, conn:{}, size:{}", m_tcp_conn, size);
	}

	LOG_INF("[owner:{}] Send tcp data, socket:{}, len:{}", m_param.owner,
		m_tcp_conn, size);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AmqpClient::onHeartbeat(Connection* connection)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	LOG_DBG("[owner:{}] {}", m_param.owner, __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AmqpClient::onError(Connection* connection, const char* message)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	m_conn_promise.set_value(false);

	LOG_INF("[owner:{}] {}", m_param.owner, __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AmqpClient::onReady(Connection* connection)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	LOG_INF("[owner:{}] {}", m_param.owner, __FUNCTION__);

	m_conn_promise.set_value(true);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AmqpClient::onClosed(Connection* connection)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	m_conn_promise.set_value(false);

	LOG_INF("[owner:{}] {}", m_param.owner, __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AmqpClient::SendAmqpHeartBeat()
{
	if (m_amqp_conn) {
		m_amqp_conn->heartbeat();
	}

	LOG_DBG("[owner:{}] {}", m_param.owner, __FUNCTION__);
}

}