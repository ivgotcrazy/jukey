#pragma once

#include <list>
#include <memory>

#include "protocol.h"
#include "transport-common.h"

namespace jukey::txp
{

//==============================================================================
// 
//==============================================================================
class IFecParamHandler
{
public:
	virtual void OnFecParamUpdate(uint8_t k, uint8_t r) = 0;
};

//==============================================================================
// 
//==============================================================================
class IFecParamController
{
public:
	virtual ~IFecParamController() {}

	virtual void OnStateFeedback(const StateFB& value) = 0;
	virtual void SetMaxProtectionRate(uint32_t rate) = 0;
};
typedef std::unique_ptr<IFecParamController> IFecParamControllerUP;

//==============================================================================
// 
//==============================================================================
class SimpleFecParamController : public IFecParamController
{
public:
	SimpleFecParamController(IFecParamHandler* handler, uint8_t k, uint8_t r);

	virtual void OnStateFeedback(const StateFB& value) override;
	virtual void SetMaxProtectionRate(uint32_t rate) override;

private:
	void TryUpdateFecParam(uint8_t k, uint8_t r);
	void UpdateStateList(const StateFB& value);
	uint32_t CalcAverRTT();
	std::pair<uint8_t, uint8_t> CalcFecParam();

private:
	IFecParamHandler* m_handler = nullptr;
	std::list<StateFB> m_state_list;

	uint8_t m_k = 0;
	uint8_t m_r = 0;

	uint32_t m_max_prot_rate = 50;

	static const uint32_t kMaxStateSize = 16;
	static const uint32_t kUpdateMinIntervalMs = 1000;
	static const uint32_t kRttThresholdMs = 2;

	uint64_t m_last_udpate_us = 0;
};

}