#pragma once

#include <thread>
#include <memory>

#include "if-thread.h"
#include "common-struct.h"
#include "msg-bus/msg-queue.h"

namespace jukey::util
{

//==============================================================================
// Thread wrapper
//==============================================================================
class CommonThread : public IThread
{
public:
	explicit CommonThread(CSTREF owner, bool batch_mode);
	explicit CommonThread(CSTREF owner, uint32_t max_que_size, bool batch_mode);
	virtual ~CommonThread();

	void StartThread();
	void StopThread();

	// IThread
	virtual bool PostMsg(const com::CommonMsg& msg) override;
	virtual void Execute(Callable callable, CallParam param) override;
	virtual com::ErrCode ExecuteSync(SyncCallableEC callable, 
		CallParam param) override;
	virtual void* ExecuteSync(SyncCallableVP callable, 
		CallParam param) override;
	
protected:
	MsgQueue<com::CommonMsg> m_msg_queue;
	volatile bool m_stop = true;

	// Overwrite this method to manage quit thread by yourself
	virtual void DoStopThread();

	bool ProcOneMsg(const com::CommonMsg& msg);

private:
	struct CallMsgData
	{
		CallMsgData(Callable c, CallParam p) : callable(c), param(p) {}

		Callable callable;
		CallParam param;
	};

	struct CallMsgDataEC
	{
		CallMsgDataEC(SyncCallableEC c, CallParam p)
			: callable(c), param(p) {}

		SyncCallableEC callable;
		CallParam param;
	};

	struct CallMsgDataVP
	{
		CallMsgDataVP(SyncCallableVP c, CallParam p)
			: callable(c), param(p) {}

		SyncCallableVP callable;
		CallParam param;
		std::promise<void*> result;
	};

private:
	// Overwrite this method to process message directly
	virtual void OnThreadMsg(const com::CommonMsg& msg);
	
	// Overwrite this method to manage message queue by yourself
	virtual void ThreadProc();

private:
	std::thread m_thread;
	std::string m_owner;
	bool m_batch_mode = true;
};

typedef std::shared_ptr<CommonThread> CommonThreadSP;

} // namespace