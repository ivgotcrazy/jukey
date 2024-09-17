#pragma once

#include <functional>
#include <memory>

#include "msg-bus.h"

namespace jukey::util
{

typedef std::function<void(const com::CommonMsg&)> CommonMsgHandler;

//==============================================================================
// 
//==============================================================================
class CommonMsgBus final : public MsgBus<CommonMsgHandler>
{
public:
	virtual void Notify(const com::CommonMsg& msg, CommonMsgHandler handler) override 
	{
		if (handler) {
			handler(msg);
		}
  }
};

}
