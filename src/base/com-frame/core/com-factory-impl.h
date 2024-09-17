#pragma once

#include <map>
#include <functional>
#include <string>
#include <mutex>

#include "com-factory.h"
#include "component.h"

#include "if-timer-mgr.h"

namespace jukey::base
{

//==============================================================================
// Implementation of component factory.
//==============================================================================
class ComFactoryImpl : public IComFactory
{
public:
	ComFactoryImpl();
	~ComFactoryImpl();

	static IComFactory* Instance();

	virtual bool Init(const std::string& com_dir) override;

	virtual IUnknown* CreateComponent(const std::string& cid,
		const std::string& owner) override;

	virtual void* QueryInterface(const std::string& cid, const std::string& iid,
		const std::string& owner) override;

	virtual void GetComponents(const std::string& iid, 
		std::vector<std::string>& cids) override;

	virtual void AddComObj(const ComObj& co) override;

	virtual void RemoveComObj(const std::string& oid) override;

	virtual std::vector<ComObj> GetComObjList(const std::string& cid) override;

private:
	void InitTimerMgr();
	void DumpComObjs();
	void TryDumpComObjs();
	void StartDumpTimer();

private:
	typedef std::string CID; // class id
	typedef std::string OID; // object id
	typedef std::map<CID, CreateComFunc> ComMap;
	typedef std::map<OID, ComObj> ComObjMap;

	std::mutex m_mutex;

	ComMap m_creators;
	ComObjMap m_com_objs;

	// Global object(singleton)
	com::ITimerMgr* m_timer_mgr = nullptr;

	uint64_t m_last_dump = 0;
	uint32_t m_accu_count = 0;
	com::TimerId m_timer_id = INVALID_TIMER_ID;
};

} // namespace