#include "stream-exchange.h"
#include "log.h"
#include "stream-server.h"


using namespace jukey::util;
using namespace jukey::com;

namespace jukey::txp
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StreamExchange::StreamExchange(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_STREAM_EXCHAGE, owner)
	, m_factory(factory)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* StreamExchange::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_STREAM_EXCHAGE) == 0) {
		return new StreamExchange(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* StreamExchange::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_STREAM_EXCHAGE)) {
		return static_cast<IStreamExchange*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamExchange::Init(IExchangeHandler* handler, IThread* thread)
{
	if (!handler) {
		LOG_ERR("Invalid handlder!");
		return ERR_CODE_INVALID_PARAM;
	}

	std::lock_guard<std::mutex> lock(m_mutex);

	m_handler = handler;
	m_thread = thread;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool StreamExchange::HasStream(const std::string& stream_id)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto item : m_stream_servers) {
		if (item.second->Stream().stream_id == stream_id) {
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool StreamExchange::HasSender(const std::string& stream_id)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto item : m_stream_servers) {
		if (item.second->SrcChannelId() != 0 &&
			item.second->Stream().stream_id == stream_id) {
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IStreamServerSP StreamExchange::FindOrCreateStreamServer(
	const MediaStream& stream, uint32_t channel_id)
{
	IStreamServerSP server;
	for (auto item : m_stream_servers) {
		if (item.second->Stream().stream_id == STRM_ID(stream)) {
			server = item.second;
			break;
		}
	}

	if (!server) {
		LOG_INF("Cannot find stream server, then create");

		server.reset(new StreamServer(m_factory, this, m_thread, stream));

		m_stream_servers.insert(std::make_pair(channel_id, server));

		LOG_INF("Add stream server to map, channel:{}", channel_id);
	}
	else {
		auto iter = m_stream_servers.find(channel_id);
		if (iter == m_stream_servers.end()) {
			m_stream_servers.insert(std::make_pair(channel_id, server));
		}
	}

	return server;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamExchange::AddSrcChannel(const MediaStream& stream, 
	uint32_t channel_id)
{
	LOG_INF("Add src channel:{}, user:{}, stream:{}|{}", channel_id, 
		stream.src.user_id,
		stream.stream.stream_type,
		stream.stream.stream_id);

	std::lock_guard<std::mutex> lock(m_mutex);

	IStreamServerSP server = FindOrCreateStreamServer(stream, channel_id);
	assert(server);
	
	if (server->SrcChannelId() != 0) {
		LOG_ERR("Has src channel, must be some thing wrong!");
		return ERR_CODE_FAILED;
	}

	ErrCode result = server->SetStreamSender(channel_id, stream.src.user_id);
	if (result != ERR_CODE_OK) {
		LOG_ERR("Set stream src failed!");
		return result; // TODO: remove server???
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamExchange::RemoveSrcChannel(uint32_t channel_id)
{
	LOG_INF("Remove src channel:{}", channel_id);

	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_stream_servers.find(channel_id);
	if (iter == m_stream_servers.end()) {
		LOG_ERR("Cannot find stream server!");
		return ERR_CODE_FAILED;
	}

	iter->second->RemoveStreamSender();

	m_stream_servers.erase(iter);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamExchange::AddDstChannel(const MediaStream& stream,
	uint32_t channel_id, uint32_t user_id)
{
	LOG_INF("Add dst channel:{}, user:{}, stream:{}|{}", channel_id, user_id,
		stream.stream.stream_type,
		stream.stream.stream_id);

	std::lock_guard<std::mutex> lock(m_mutex);

	IStreamServerSP server = FindOrCreateStreamServer(stream, channel_id);
	assert(server);

	if (server->HasDstChannel(channel_id)) {
		LOG_ERR("Dst channel exists");
		return ERR_CODE_FAILED;
	}

	ErrCode result = server->AddStreamReceiver(channel_id, user_id);
	if (result != ERR_CODE_OK) {
		LOG_ERR("Add stream dst failed!");
		return result; // TODO: remove server???
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamExchange::RemoveDstChannel(uint32_t channel_id,
	const std::string& stream_id)
{
	LOG_INF("Remove dst channel:{}, stream_id:{}", channel_id, stream_id);

	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_stream_servers.find(channel_id);
	if (iter == m_stream_servers.end()) {
		LOG_ERR("Cannot find stream!");
		return ERR_CODE_FAILED;
	}

	iter->second->RemoveStreamReceiver(channel_id);

	m_stream_servers.erase(iter);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamExchange::OnRecvChannelData(uint32_t channel_id, uint32_t mt,
	const com::Buffer& buf)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_stream_servers.find(channel_id);
	if (iter != m_stream_servers.end()) {
		return iter->second->OnRecvChannelData(channel_id, mt, buf);
	}
	else {
		LOG_ERR("Cannot find stream server by channel:{}", channel_id);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamExchange::OnRecvChannelMsg(uint32_t channel_id, const com::Buffer& buf)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_stream_servers.find(channel_id);
	if (iter != m_stream_servers.end()) {
		return iter->second->OnRecvChannelMsg(channel_id, buf);
	}
	else {
		LOG_ERR("Cannot find stream server by channel:{}", channel_id);
	}
}

//------------------------------------------------------------------------------
// IServerHandler
//------------------------------------------------------------------------------
void StreamExchange::OnSendChannelMsg(uint32_t channel_id, uint32_t user_id,
	const Buffer& buf)
{
	m_handler->OnSendChannelMsg(channel_id, user_id, buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamExchange::OnSendChannelData(uint32_t channel_id, uint32_t user_id,
	uint32_t mt, const com::Buffer& buf)
{
	m_handler->OnSendChannelData(channel_id, user_id, mt, buf);
}

}