#include "sending-queue.h"
#include "event/common-event.h"
#include "log.h"
#include "common/util-time.h"


using namespace jukey::util;

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SendingQueue::SendingQueue()
{
	m_sending_event.reset(new CommonEvent());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ISendEntrySP SendingQueue::GetNextEntry()
{
	m_mutex.lock();
	if (m_entry_heap.empty()) {
		m_sending_event->Reset();
		m_mutex.unlock();
		m_sending_event->Wait(WAIT_INFINITE);
	}
	else {
		m_mutex.unlock();
	}

	m_mutex.lock();
	if (m_entry_heap.empty()) {
		LOG_ERR("Empty entry heap!");
		m_mutex.unlock();
		return nullptr;
	}

	ISendEntrySP entry = m_entry_heap.front();
	PopEntry(entry->GetEntryId());
	m_mutex.unlock();

	uint64_t now = util::Now();
	uint64_t next_send_time = entry->NextSendTime();
	if (next_send_time == INVALID_SEND_TIME) {
		entry.reset();
	}
	else {
		if (next_send_time > now) {
			util::Sleep(next_send_time - now);
		}
	}

	return entry;
}

//------------------------------------------------------------------------------
// Lock outside
//------------------------------------------------------------------------------
void SendingQueue::PopEntry(uint64_t entry_id)
{
	// Remove from heap
	std::pop_heap(m_entry_heap.begin(), m_entry_heap.end());
	m_entry_heap.pop_back();

	// Remove from map
	m_entry_map.erase(entry_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SendingQueue::AddEntry(ISendEntrySP entry)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// It's very likely that sending thread and protocol thread add the same entry
	// at the same time
	if (m_entry_map.find(entry->GetEntryId()) != m_entry_map.end()) {
		LOG_DBG("Add existing entry:{}!", entry->GetEntryId());
		return;
	}

	// Add to heap
	m_entry_heap.push_back(entry);
	std::push_heap(m_entry_heap.begin(), m_entry_heap.end());

	// Add to map
	m_entry_map.insert(std::make_pair(entry->GetEntryId(), entry));

	LOG_DBG("Add entry:{}, map size:{}, heap size:{}", entry->GetEntryId(),
		m_entry_map.size(), m_entry_heap.size());

	m_sending_event->Trigger();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SendingQueue::RemoveEntry(uint64_t entry_id)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// Try to remove entry from map
	if (m_entry_map.find(entry_id) == m_entry_map.end()) {
		LOG_DBG("Cannot find entry:{} in map, map size:{}, heap size:{}", entry_id,
			m_entry_heap.size(), m_entry_heap.size());
		return;
	}
	else {
		m_entry_map.erase(entry_id);
		LOG_INF("Remove entry:{} from map, map size:{}", entry_id, m_entry_map.size());
	}

	// Remove from heap
	for (auto iter = m_entry_heap.begin(); iter != m_entry_heap.end(); iter++) {
		if ((*iter)->GetEntryId() == entry_id) {
			m_entry_heap.erase(iter);
			std::make_heap(m_entry_heap.begin(), m_entry_heap.end());
			LOG_INF("Remove entry:{} from heap, head size:{}", entry_id,
				m_entry_heap.size());
			return;
		}
	}

	LOG_ERR("Remove entry:{} from entry heap failed!", entry_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SendingQueue::UpdateEntry(ISendEntrySP entry, bool force)
{
	m_mutex.lock();
	if (m_entry_map.find(entry->GetEntryId()) != m_entry_map.end()) {
		m_mutex.unlock();
		if (force) {
			RemoveEntry(entry->GetEntryId());
			AddEntry(entry);
		}
	}
	else {
		m_mutex.unlock();
		AddEntry(entry);
	}
}

}