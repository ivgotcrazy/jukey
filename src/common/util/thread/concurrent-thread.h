#pragma once

#include <thread>
#include <memory>

#include "if-thread.h"
#include "common-struct.h"
#include "blockingconcurrentqueue.h"

using namespace moodycamel;

namespace jukey::util
{

//==============================================================================
// Thread wrapper
//==============================================================================
class ConcurrentThread : public IThread
{
public:
	explicit ConcurrentThread(CSTREF owner);
	explicit ConcurrentThread(CSTREF owner, uint32_t max_que_size);
	virtual ~ConcurrentThread();

	void StartThread();
	void StopThread();

	// IThread
	virtual bool PostMsg(const com::CommonMsg& msg) override;
	virtual void Execute(Callable callable, CallParam param) override {};
	virtual com::ErrCode ExecuteSync(SyncCallableEC callable, CallParam param) override 
	{
		return com::ERR_CODE_OK;
	};
	virtual void* ExecuteSync(SyncCallableVP callable, CallParam param) override 
	{
		return nullptr;
	};
	
protected:
	BlockingConcurrentQueue<com::CommonMsg> m_msg_queue;
	volatile bool m_stop = false;

	const uint32_t QUIT_THREAD_MSG = 0x66778899;

private:
	// Overwrite this method to process message directly
	virtual void OnThreadMsg(const com::CommonMsg& msg);
	
	// Overwrite this method to manage message queue by yourself
	virtual void ThreadProc();

private:
	std::thread m_thread;
	std::string m_owner;
};
typedef std::shared_ptr<ConcurrentThread> ConcurrentThreadSP;

} // namespace