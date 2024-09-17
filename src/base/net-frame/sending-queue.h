#pragma once

#include <mutex>
#include <unordered_map>

#include "if-sending-queue.h"
#include "event/if-event.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class SendingQueue : public ISendQueue
{
public:
	SendingQueue();

	// ISendQueue
	virtual ISendEntrySP GetNextEntry() override;
	virtual void AddEntry(ISendEntrySP entry) override;
	virtual void RemoveEntry(uint64_t entry_id) override;
	virtual void UpdateEntry(ISendEntrySP entry, bool force) override;

private:
	void PopEntry(uint64_t entry_id);

private:
	util::IEventSP m_sending_event;
	std::vector<ISendEntrySP> m_entry_heap;
	std::unordered_map<uint64_t, ISendEntrySP> m_entry_map;
	std::mutex m_mutex;
};

}