#pragma once

#include "msg-bus.h"

namespace jukey::util
{

//==============================================================================
// Messages will be post to thread
//==============================================================================
class ThreadMsgBus final : public MsgBus<CommonThread*>
{
public:
	virtual void Notify(const com::CommonMsg& msg, CommonThread* handler) override
	{
		if (handler) {
			handler->PostMsg(msg);
		}
  }
};

}
