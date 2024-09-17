#pragma once

#include "if-session.h"

namespace jukey::net
{

#define INVALID_SEND_TIME 0xFFFFFFFFFFFFFFFF

//==============================================================================
// 
//==============================================================================
class ISendEntry
{
public:
	virtual ~ISendEntry() {}

	virtual uint64_t GetEntryId() = 0;

	virtual uint64_t NextSendTime() = 0;

	virtual void SendData() = 0;
};

typedef std::shared_ptr<ISendEntry> ISendEntrySP;

//==============================================================================
// 
//==============================================================================
class ISendQueue
{
public:
	virtual ~ISendQueue() {}

	virtual ISendEntrySP GetNextEntry() = 0;

	virtual void AddEntry(ISendEntrySP entry) = 0;

	virtual void RemoveEntry(uint64_t entry_id) = 0;

	virtual void UpdateEntry(ISendEntrySP entry, bool force) = 0;
};

typedef std::shared_ptr<ISendQueue> ISendQueueSP;
typedef std::unique_ptr<ISendQueue> ISendQueueUP;

}