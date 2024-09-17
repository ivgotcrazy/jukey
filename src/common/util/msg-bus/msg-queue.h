#pragma once

#include <mutex>
#include <list>
#include <condition_variable>

#include "common-struct.h"

namespace jukey::util
{

//==============================================================================
// Simple mesage queue
//==============================================================================
template <typename T>
class MsgQueue
{
public:
	MsgQueue(CSTREF owner, uint32_t max_que_size = 4096);

	bool PushMsg(const T& msg);

	bool PopMsg(T& msg);

	std::list<T> PopAllMsg();

	bool IsEmpty();

	uint32_t GetQueueSize();

private:
	std::list<T> m_msg_que;

	std::mutex m_que_mutex;
	std::condition_variable m_con_var;

	uint32_t m_max_que_size;

	std::string m_owner;
};

}
#include "msg-queue-impl.hpp"
