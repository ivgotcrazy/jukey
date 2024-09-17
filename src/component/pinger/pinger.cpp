#include "pinger.h"
#include "log.h"
#include "common/util-time.h"
#include "protocol.h"
#include "topo-msg-builder.h"
#include "protoc/topo.pb.h"
#include "pinger-common.h"

using namespace jukey::com;

namespace jukey::com
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Pinger::Pinger(base::IComFactory* factory, const char* owner)
	: base::ProxyUnknown(nullptr)
	, base::ComObjTracer(factory, CID_PINGER, owner)
	, util::CommonThread("pinger", true)
	, m_factory(factory)
{
	
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* Pinger::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_PINGER) == 0) {
		return new Pinger(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* Pinger::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_PINGER)) {
		return static_cast<IPinger*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool Pinger::Init(const ServiceParam& param, IPingHandler* handler,
	uint32_t ping_interval)
{
	// TODO: param check
	m_param = param;
	m_handler = handler;
	m_ping_interval = ping_interval * 1000;

	m_timer_mgr = QUERY_TIMER_MGR(m_factory);
	if (!m_timer_mgr) {
		LOG_ERR("get timer from factory failed");
		return false;
	}

	LOG_INF("Init success");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Pinger::OnRecvPongMsg(const com::Buffer& buf)
{
	prot::Pong msg;
	if (!msg.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse pong message failed!");
		return;
	}

	std::lock_guard<std::mutex> m_lock(m_mutex);

	bool found = false;
	for (auto& entry : m_ping_entries) {
		if (entry.service.service_type == msg.service_type()) {
			if (entry.service.instance_id.empty() 
				|| entry.service.instance_id == msg.instance_id()) {
				LOG_INF("Notify ping success, type:{}, name:{}, instance:{}",
					entry.service.service_type,
					entry.service.service_name,
					entry.service.instance_id);
				m_handler->OnPingResult(entry.service, true);
				entry.last_ping = 0;
				found = true;
				break;
			}
		}
	}

	if (!found) {
		LOG_WRN("cannot find service for pong msg, type:{}, name:{}, instance:{}",
			msg.service_type(), msg.service_name(), msg.instance_id());
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Pinger::AddPingService(const ServiceParam& param)
{
	std::lock_guard<std::mutex> m_lock(m_mutex);

	for (auto& entry : m_ping_entries) {
		if (entry.service.service_type == param.service_type
			&& entry.service.instance_id == param.instance_id) {
			LOG_ERR("service exists already, type:{}, instance:{}", 
				entry.service.service_type, entry.service.instance_id);
			return;
		}
	}

	m_ping_entries.push_back(PingEntry{ param, 0, 0 });
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Pinger::StartPingTimer()
{
	com::TimerParam timer_param;
	timer_param.timeout    = m_ping_interval + 1000;
	timer_param.timer_type = com::TimerType::TIMER_TYPE_LOOP;
	timer_param.timer_name = "ping timer";
	timer_param.run_atonce = true;
	timer_param.timer_func = [this](int64_t) {
		this->PostMsg(jukey::com::CommonMsg(jukey::com::PINGER_MSG_PING_TIMER));
	};

	m_ping_timer = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(m_ping_timer);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Pinger::StartTimeoutTimer()
{
	com::TimerParam timer_param;
	timer_param.timeout    = m_ping_interval;
	timer_param.timer_type = com::TimerType::TIMER_TYPE_ONCE;
	timer_param.timer_name = "ping timeout timer";
	timer_param.timer_func = [this](int64_t) {
		this->PostMsg(jukey::com::CommonMsg(jukey::com::PINGER_MSG_CHECK_TIMER));
	};
	
	m_timeout_timer = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(m_timeout_timer);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Pinger::Start()
{
	StartThread();

	StartPingTimer();

	LOG_INF("Start pinger, service type:{}, service name:{}, instance:{}",
		m_param.service_type, m_param.service_name, m_param.instance_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Pinger::Stop()
{
	if (m_ping_timer != INVALID_TIMER_ID) {
		m_timer_mgr->StopTimer(m_ping_timer);
		m_timer_mgr->FreeTimer(m_ping_timer);
		m_ping_timer = INVALID_TIMER_ID;
	}

	if (m_timeout_timer != INVALID_TIMER_ID) {
		m_timer_mgr->StopTimer(m_timeout_timer);
		m_timer_mgr->FreeTimer(m_timeout_timer);
		m_timeout_timer = INVALID_TIMER_ID;
	}

	StopThread();

	LOG_INF("Stop pinger, service type:{}, service name:{}, instance:{}",
		m_param.service_type, m_param.service_name, m_param.instance_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Pinger::SendPingMsg()
{
	std::lock_guard<std::mutex> m_lock(m_mutex);

	for (auto& entry : m_ping_entries) {
		com::Buffer buf = prot::util::BuildPingMsg(++entry.last_seq,
			entry.service.service_name,
			entry.service.service_type,
			entry.service.instance_id);
		m_handler->SendPing(entry.service, buf);

		entry.last_ping = util::Now();
	}

	StartTimeoutTimer();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Pinger::CheckTimeout()
{
	std::lock_guard<std::mutex> m_lock(m_mutex);

	for (auto& entry : m_ping_entries) {
		if (entry.last_ping != 0
			&& entry.last_ping + (m_ping_interval + 1000) * 1000 < util::Now()) {
			LOG_ERR("Notify ping failed, type:{}, name:{}, instance:{}",
				entry.service.service_type,
				entry.service.service_name,
				entry.service.instance_id);
			m_handler->OnPingResult(entry.service, false);
			entry.last_ping = 0;
		}
	}

	m_timer_mgr->FreeTimer(m_timeout_timer);
	m_timeout_timer = INVALID_TIMER_ID;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Pinger::OnThreadMsg(const com::CommonMsg& msg)
{
	switch (msg.msg_type) {
	case PINGER_MSG_PING_TIMER:
		SendPingMsg();
		break;
	case PINGER_MSG_CHECK_TIMER:
		CheckTimeout();
		break;
	default:
		LOG_ERR("Unknown message type:{}", msg.msg_type);
	}
}

}