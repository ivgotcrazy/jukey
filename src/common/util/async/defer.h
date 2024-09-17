#pragma once

#include <memory>
#include <functional>

#include "common-struct.h"
#include "net-public.h"

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class Defer
{
public:
	using TimeoutCallback = std::function<void()>;
	using ErrorCallback = std::function<void(const std::string&)>;

public:
	virtual ~Defer() {}

	Defer& OnError(ErrorCallback callback)
	{
		m_error_callback = std::move(callback);
		return *this;
	}

	Defer& OnTimeout(TimeoutCallback callback)
	{
		m_timeout_callback = std::move(callback);
		return *this;
	}

	void ReportTimeout()
	{
		if (m_timeout_callback) m_timeout_callback();
	}

	void ReportError(const std::string& err)
	{
		if (m_error_callback) m_error_callback(err);
	}

	void SetTimerId(uint64_t timer_id)
	{
		m_timer_id = timer_id;
	}

	uint64_t TimerId()
	{
		return m_timer_id;
	}

private:
	uint64_t m_timer_id = 0;
	TimeoutCallback m_timeout_callback;
	ErrorCallback m_error_callback;
};
typedef std::shared_ptr<Defer> DeferSP;

//==============================================================================
// 
//==============================================================================
class MqDefer : public Defer
{
public:
	using RspCallback = std::function<void(const com::Buffer&, const com::Buffer&)>;

public:
	MqDefer& OnResponse(RspCallback callback)
	{
		m_rsp_callback = std::move(callback);
		return *this;
	}

	void ReportResponse(const com::Buffer& mq_buf, const com::Buffer& sig_buf)
	{
		if (m_rsp_callback) m_rsp_callback(mq_buf, sig_buf);
	}

private:
	RspCallback m_rsp_callback;
};
typedef std::shared_ptr<MqDefer> MqDeferSP;

//==============================================================================
// 
//==============================================================================
class SessionDefer : public Defer
{
public:
	using RspCallback = std::function<void(net::SessionId sid, const com::Buffer&)>;

public:
	SessionDefer& OnResponse(RspCallback callback)
	{
		m_rsp_callback = std::move(callback);
		return *this;
	}

	void ReportResponse(net::SessionId sid, const com::Buffer& buf)
	{
		if (m_rsp_callback) m_rsp_callback(sid, buf);
	}

private:
	RspCallback m_rsp_callback;
};
typedef std::shared_ptr<SessionDefer> SessionDeferSP;

}