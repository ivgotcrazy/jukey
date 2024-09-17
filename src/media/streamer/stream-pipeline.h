#pragma once

#include <vector>
#include <mutex>
#include <map>

#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "if-pipeline.h"
#include "if-sync-mgr.h"
#include "if-element.h"
#include "if-pin.h"
#include "msg-bus/thread-msg-bus.h"
#include "msg-bus/common-msg-bus.h"
#include "thread/common-thread.h"
#include "element-assembler.h"
#include "pipeline-msg.h"

namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class StreamPipeline 
	: public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
	, public IPipeline
{
public:
	StreamPipeline(base::IComFactory* factory, const char* owner);
	~StreamPipeline();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IPipeline
	virtual com::ErrCode Init(CSTREF pipeline_name) override;
	virtual ISyncMgr& GetSyncMgr() override;
	virtual std::string Name() override;
	virtual com::ErrCode Start() override;
	virtual com::ErrCode Pause() override;
	virtual com::ErrCode Resume() override;
	virtual com::ErrCode Stop() override;
	virtual PipelineState State() override;
	virtual IElement* AddElement(CSTREF cid, com::IProperty* props) override;
	virtual com::ErrCode RemoveElement(CSTREF name) override;
	virtual void PostPlMsg(const com::CommonMsg& msg) override;
	virtual com::ErrCode SendPlMsg(const com::CommonMsg& msg) override;
	virtual com::ErrCode SubscribeMsg(uint32_t msg_type,
		IPlMsgHandler* handler) override;
	virtual com::ErrCode UnsubscribeMsg(uint32_t msg_type,
		IPlMsgHandler* handler) override;
	virtual com::ErrCode LinkElement(ISrcPin* src_pin,
		ISinkPin* sink_pin) override;
	virtual IElement* GetElementByName(CSTREF name) override;

	// CommonThread
	virtual void OnThreadMsg(const com::CommonMsg& msg) override;

	void DumpPipeline();

private:
	void OnAddElementStream(const com::CommonMsg& msg);
	void OnRemoveElementStream(const com::CommonMsg& msg);
	void HandleMsgBusMsg(const com::CommonMsg& msg);

	bool HasSrcElement();
	bool DoStartElements();
	bool DoStopElements();
	bool DoPauseElements();
	bool DoResumeElements();

private:
	base::IComFactory* m_factory = nullptr;

	std::string m_pipeline_name;

	// Audio and video synchrolization
	ISyncMgr& m_sync_mgr;

	//util::ThreadMsgBus m_msg_bus;
	util::CommonMsgBus m_msg_bus;
	
	PipelineState m_state = PipelineState::PIPELINE_STATE_INVALID;

	// key: element name
	std::map<std::string, IElement*> m_elements;

	// key: stream id
	std::map<std::string, std::vector<com::ElementStream>> m_streams;
};

}