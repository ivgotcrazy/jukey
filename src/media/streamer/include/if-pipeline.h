#pragma once

#include "component.h"
#include "com-factory.h"
#include "common-enum.h"
#include "common-struct.h"
#include "if-property.h"
#include "if-sync-mgr.h"

#include "public/media-enum.h"

namespace jukey::stmr
{

// Component ID and interface ID
#define CID_PIPELINE "cid-pipeline"
#define IID_PIPELINE "iid-pipeline"

//==============================================================================
// Pipeline message handler
//==============================================================================
class IPlMsgHandler
{
public:
	//
	// @brief Received pipeline message 
	//
	virtual void OnPipelineMsg(const com::CommonMsg& msg) = 0;
};

//==============================================================================
// -----INITED
// |      |
// |stop  |start
// |      ↓
// |----RUNNING<--|
// |      |       |
// |stop  |pause  |resume
// |      ↓       |
// |    PAUSED----|
// |      |
// |      |stop
// |      ↓
// |--->STOPED
//==============================================================================
enum class PipelineState
{
	PIPELINE_STATE_INVALID = 0,
	PIPELINE_STATE_INITED  = 1,
	PIPELINE_STATE_PAUSED  = 2,
	PIPELINE_STATE_RUNNING = 3,
	PIPELINE_STATE_STOPED  = 4,
};

class ISyncMgr;
class IElement;
class ISrcPin;
class ISinkPin;
struct EleStreamData;
//==============================================================================
// Interface of pipeline, which is a component
//==============================================================================
class IPipeline : public base::IUnknown
{
public:
	/**
	 * @brief Initialize 
	 */
	virtual com::ErrCode Init(const std::string& pipeline_name) = 0;

	/**
	 * @brief Manager of audio and video synchronization
	 */
	virtual ISyncMgr& GetSyncMgr() = 0;

	/**
	 * @brief Get pipeline name
	 */
	virtual std::string Name() = 0;

	/**
	 * @brief Start pipeline
	 */
	virtual com::ErrCode Start() = 0;

	/**
	 * @brief Pause pipeline
	 */
	virtual com::ErrCode Pause() = 0;

	/**
	 * @brief Resume pipeline
	 */
	virtual com::ErrCode Resume() = 0;

	/**
	 * @brief Stop pipeline
	 */
	virtual com::ErrCode Stop() = 0;

	/**
	 * @brief Get pipeline state
	 */
	virtual PipelineState State() = 0;

	/**
	 * @brief Add element into pipeline
	 */
	virtual IElement* AddElement(const std::string& cid, 
		com::IProperty* props) = 0;

	/**
	 * @brief Remove element from pipeline
	 */
	virtual com::ErrCode RemoveElement(const std::string& ele_name) = 0;

	/**
	 * @brief Post pipeline message asynchronously
	 */
	virtual void PostPlMsg(const com::CommonMsg& msg) = 0;

	/**
	 * @brief Send pipeline message synchronously
	 */
	virtual com::ErrCode SendPlMsg(const com::CommonMsg& msg) = 0;

	/**
	 * @brief Subscrible pipeline message
	 */
	virtual com::ErrCode SubscribeMsg(uint32_t msg_type, 
		IPlMsgHandler* handler) = 0;

	/**
	 * @brief Unsubscrible pipeline message
	 */
	virtual com::ErrCode UnsubscribeMsg(uint32_t msg_type, 
		IPlMsgHandler* handler) = 0;

	/**
	 * @brief Link element src pin and sink pin
	 */
	virtual com::ErrCode LinkElement(ISrcPin* src_pin, ISinkPin* sink_pin) = 0;

	/**
	 * @brief Get element by name
	 */
	virtual IElement* GetElementByName(const std::string& ele_name) = 0;
};

}