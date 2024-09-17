#include "common-thread.h"
#include "log/util-log.h"
#include "common-config.h"

#define QUIT_THREAD_MSG      0x66778899
#define EXECUTE_ASYNC        0x6677889A
#define EXECUTE_SYNC_VOIDPTR 0x6677889B
#define EXECUTE_SYNC_ERRCODE 0x6677889C


namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
CommonThread::CommonThread(CSTREF owner, bool batch_mode)
	: m_msg_queue(owner)
	, m_owner(owner)
	, m_batch_mode(batch_mode)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
CommonThread::CommonThread(CSTREF owner, uint32_t max_que_size, bool batch_mode)
	: m_msg_queue(owner, max_que_size)
	, m_owner(owner)
	, m_batch_mode(batch_mode)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
CommonThread::~CommonThread()
{
	StopThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool CommonThread::PostMsg(const com::CommonMsg& msg)
{
	return m_msg_queue.PushMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CommonThread::Execute(Callable callable, CallParam param)
{
	std::shared_ptr<CallMsgData> data(new CallMsgData(callable, param));

	com::CommonMsg msg;
	msg.msg_type = EXECUTE_ASYNC;
	msg.msg_data = data;

	m_msg_queue.PushMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode CommonThread::ExecuteSync(SyncCallableEC callable, CallParam param)
{
	std::shared_ptr<CallMsgDataEC> data(new CallMsgDataEC(callable, param));

	com::CommonMsg msg;
	msg.msg_type = EXECUTE_SYNC_ERRCODE;
	msg.msg_data = data;
	msg.result.reset(new std::promise<jukey::com::ErrCode>());

	m_msg_queue.PushMsg(msg);

	return msg.result->get_future().get();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* CommonThread::ExecuteSync(SyncCallableVP callable, CallParam param)
{
	std::shared_ptr<CallMsgDataVP> data(new CallMsgDataVP(callable, param));

	com::CommonMsg msg;
	msg.msg_type = EXECUTE_SYNC_VOIDPTR;
	msg.msg_data = data;

	m_msg_queue.PushMsg(msg);

	return data->result.get_future().get();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CommonThread::StartThread()
{
	UTIL_INF("Start thread:{}", m_owner);

	m_stop = false;
	m_thread = std::thread(&CommonThread::ThreadProc, this);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CommonThread::DoStopThread()
{
	UTIL_INF("Do Stop thread:{}", m_owner);

	PostMsg(com::CommonMsg(QUIT_THREAD_MSG, 1));
	m_stop = true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CommonThread::StopThread()
{
	UTIL_INF("Stop thread:{}", m_owner);

	if (m_stop) return;

	DoStopThread();

	if (m_thread.joinable()) {
		UTIL_INF("Join thread begin:{}", m_owner);
		m_thread.join();
		UTIL_INF("Join thread end:{}", m_owner);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool CommonThread::ProcOneMsg(const com::CommonMsg& msg)
{
	if (msg.msg_type == QUIT_THREAD_MSG) {
		UTIL_INF("Quit thread!");
		return false;
	}
	else if (msg.msg_type == EXECUTE_ASYNC) {
		PCAST_COMMON_MSG_DATA(CallMsgData);
		data->callable(data->param);
	}
	else if (msg.msg_type == EXECUTE_SYNC_ERRCODE) {
		PCAST_COMMON_MSG_DATA(CallMsgDataEC);
		com::ErrCode result = data->callable(data->param);
		if (msg.result) {
			msg.result->set_value(result);
		}
	}
	else if (msg.msg_type == EXECUTE_SYNC_VOIDPTR) {
		PCAST_COMMON_MSG_DATA(CallMsgDataVP);
		void* p = data->callable(data->param);
		data->result.set_value(p);
	}
	else {
		OnThreadMsg(msg);
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CommonThread::ThreadProc()
{
	UTIL_INF("Enter thread:{}", m_owner);

	std::list<com::CommonMsg> msg_list;
	while (!m_stop) {
		if (m_batch_mode) {
			msg_list = m_msg_queue.PopAllMsg();
			for (auto& msg : msg_list) {
				if (!ProcOneMsg(msg)) {
					goto EXIT;
				}
			}
			msg_list.clear();
		}
		else {
			com::CommonMsg msg;
			if (m_msg_queue.PopMsg(msg)) {
				if (!ProcOneMsg(msg)) {
					break;
				}
			}
		}
	}

EXIT:
	UTIL_INF("Exit thread:{}", m_owner);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CommonThread::OnThreadMsg(const com::CommonMsg& msg)
{
	UTIL_INF("OnThreadMsg, msg[{}]", msg.msg_type);
}

}