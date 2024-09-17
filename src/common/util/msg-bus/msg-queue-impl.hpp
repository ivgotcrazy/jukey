#pragma once

#include "log/util-log.h"
#include "common-struct.h"

#define WAIT_DATA_TIME_MS 1

namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <typename T>
MsgQueue<T>::MsgQueue(CSTREF owner, uint32_t max_que_size)
	: m_max_que_size(max_que_size)
	, m_owner(owner)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <typename T>
bool MsgQueue<T>::PushMsg(const T& msg)
{
	std::unique_lock<std::mutex> lock(m_que_mutex);

	if (m_msg_que.size() >= m_max_que_size) {
		UTIL_WRN("Message queue of {} is full!", m_owner);
		return false;
	}

	if (msg.pri == 0) {
		m_msg_que.push_back(msg);
	}
	else { // TODO: priority
		m_msg_que.push_front(msg);
	}

	UTIL_DBG("{} message queue size:{}", m_owner, m_msg_que.size());

	m_con_var.notify_all();

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <typename T>
bool MsgQueue<T>::PopMsg(T& msg)
{
	std::unique_lock<std::mutex> lock(m_que_mutex);

	m_con_var.wait_for(lock, std::chrono::milliseconds(WAIT_DATA_TIME_MS), 
		[this]() { return !m_msg_que.empty(); });

	if (!m_msg_que.empty()) {
		msg = m_msg_que.front();
		m_msg_que.pop_front();
		return true;
	}
	else {
		return false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <typename T>
std::list<T> MsgQueue<T>::PopAllMsg()
{
	std::unique_lock<std::mutex> lock(m_que_mutex);

	m_con_var.wait_for(lock, std::chrono::milliseconds(WAIT_DATA_TIME_MS), 
		[this]() { return !m_msg_que.empty(); });

	std::list<T> msg_list;
	if (!m_msg_que.empty()) {
		msg_list.swap(m_msg_que);
	}

	return msg_list;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <typename T>
bool MsgQueue<T>::IsEmpty()
{
	return m_msg_que.empty();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <typename T>
uint32_t MsgQueue<T>::GetQueueSize()
{
	return (uint32_t)m_msg_que.size();
}

}
