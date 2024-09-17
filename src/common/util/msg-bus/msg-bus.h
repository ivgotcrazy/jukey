#pragma once

#include <map>
#include <set>
#include <mutex>

#include "msg-queue.h"
#include "thread/common-thread.h"

namespace jukey::util
{

//==============================================================================
// @brief 消息总线，支持发布和订阅消息，消息总线使用自己的处理线程
// @param HT - Handler Type
//==============================================================================
template <class HT>
class MsgBus : public CommonThread
{
public:
	MsgBus() : CommonThread("MsgBus", true), m_msg_queue("MsgBus") {}

	//
	// Start message bus
	//
	void Start();

	//
	// Stop message bus
	//
	void Stop();

	//
	// Publish message
	//
	bool PublishMsg(const com::CommonMsg& msg);

	//
	// Subscribe designated message
	//
	void SubscribeMsg(uint32_t msg_type, HT msg_handler, void* obj);

	//
	// Cancel subscribe designated message
	//
	void UnsubscribeMsg(uint32_t msg_type, void* obj);

	//
	// Subscribe all messages
	//
	void SubscribeMsg(HT msg_handler, void* obj);

	//
	// Cancel subscribe all messages
	//
	void UnsubscribeMsg(void* obj);

private:
	// Message notify implementation
	virtual void Notify(const com::CommonMsg& msg, HT handler) = 0;

	// CommonThread
	virtual void ThreadProc() override;

	void DispatchMsg(const com::CommonMsg& msg);

	template<class T>
	struct HandlerEntry
	{
		HandlerEntry(T h, void* o) : handler(h), obj(o) {}

		T handler;
		void* obj = nullptr;
	};

	template<class T>
	struct Comparator
	{
		bool operator()(const T& lhs, const T& rhs) const
		{
			return lhs.obj < rhs.obj;
		}
	};

private:
	typedef std::set<HandlerEntry<HT>, Comparator<HandlerEntry<HT>>> MsgHandlerSet;
	typedef std::map<uint32_t, MsgHandlerSet> MsgHandlerMap;

	// Handle one designated message
	MsgHandlerMap m_msg_handlers;

	// Handle all messages， bc: broadcast
	MsgHandlerSet m_bc_handlers;

	std::mutex m_handler_mutex;

	MsgQueue<com::CommonMsg> m_msg_queue;
};

}
#include "msg-bus-impl.hpp"
