#pragma once

#include "msg-bus.h"

#define QUIT_THREAD_MSG 0x66778899

namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <class HT>
void MsgBus<HT>::Start()
{
	StartThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <class HT>
void MsgBus<HT>::Stop()
{
	// 使用自己队列，没有使用CommonThread的队列，因此需要自己发送线程结束消息
	m_msg_queue.PushMsg(com::CommonMsg(QUIT_THREAD_MSG));

	StopThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <class HT>
bool MsgBus<HT>::PublishMsg(const com::CommonMsg& msg)
{
	return m_msg_queue.PushMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <class HT>
void MsgBus<HT>::SubscribeMsg(uint32_t msg_type, HT msg_handler, void* obj)
{
	std::unique_lock<std::mutex> lock(m_handler_mutex);

	auto iter = m_msg_handlers.find(msg_type);
	if (iter == m_msg_handlers.end()) {
		MsgHandlerSet handlers;
		handlers.insert(HandlerEntry(msg_handler, obj));
		m_msg_handlers.insert(std::make_pair(msg_type, handlers));
	} else {
		iter->second.insert(HandlerEntry(msg_handler, obj));
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <class HT>
void MsgBus<HT>::UnsubscribeMsg(uint32_t msg_type, void* obj)
{
	std::unique_lock<std::mutex> lock(m_handler_mutex);

	auto iter = m_msg_handlers.find(msg_type);
	if (iter != m_msg_handlers.end()) {
		iter->second.erase(HandlerEntry(HT(), obj));
	}
	else {
		UTIL_ERR("Cannot find mesage type {}", msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <class HT>
void MsgBus<HT>::SubscribeMsg(HT msg_handler, void* obj)
{
	std::unique_lock<std::mutex> lock(m_handler_mutex);

	m_bc_handlers.insert(HandlerEntry(msg_handler, obj));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <class HT>
void MsgBus<HT>::UnsubscribeMsg(void* obj)
{
	std::unique_lock<std::mutex> lock(m_handler_mutex);

	m_bc_handlers.erase(HandlerEntry(HT(), obj));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <class HT>
void MsgBus<HT>::DispatchMsg(const com::CommonMsg& msg)
{
	std::unique_lock<std::mutex> lock(m_handler_mutex);

	// Designated subscribe dispatch
	auto iter = m_msg_handlers.find(msg.msg_type);
	if (iter != m_msg_handlers.end()) {
		MsgHandlerSet& handlers = iter->second;
		for (auto& entry : handlers) {
			Notify(msg, entry.handler);
		}
	}

	// Broadcast subscribe dispatch
	for (auto& entry : m_bc_handlers) {
		Notify(msg, entry.handler);
	}

	UTIL_INF("Remain msg count:{}", m_msg_queue.GetQueueSize());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
template <class HT>
void MsgBus<HT>::ThreadProc()
{
	while (!m_stop) {
		com::CommonMsg msg;
		if (m_msg_queue.PopMsg(msg)) {
			if (msg.msg_type == QUIT_THREAD_MSG) {
				UTIL_INF("Quit thread!");
				break;
			}
			DispatchMsg(msg);
		}
	}
}

}
