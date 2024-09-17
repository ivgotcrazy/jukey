#pragma once

#include <thread>
#include <memory>
#include <functional>

#include "common-struct.h"
#include "msg-bus/msg-queue.h"

namespace jukey::util
{

typedef std::shared_ptr<void> CallParam;

typedef std::function<void(CallParam)> Callable;

typedef std::function<com::ErrCode(CallParam)> SyncCallableEC;

typedef std::function<void*(CallParam)> SyncCallableVP;

//==============================================================================
// 
//==============================================================================
class IThread
{
public:
	virtual bool PostMsg(const com::CommonMsg& msg) = 0;

	virtual void Execute(Callable callable, CallParam param) = 0;

	virtual com::ErrCode ExecuteSync(SyncCallableEC callable, CallParam param) = 0;

	virtual void* ExecuteSync(SyncCallableVP callable, CallParam param) = 0;
};

} // namespace