#pragma once

#include "net-common.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
struct LossInfo
{
	LossInfo() {}
	LossInfo(uint32_t sl, uint32_t fl) : ses_loss(sl), fec_loss(fl) {}

	uint32_t ses_loss = 0;
	uint32_t fec_loss = 0;
};

//==============================================================================
// 
//==============================================================================
class IFecController
{
public:
	virtual ~IFecController() {}

	virtual void UpdateLossInfo(const LossInfo& info) = 0;

	virtual FecParam GetFecParam() = 0;
};
typedef std::unique_ptr<IFecController> IFecControllerUP;

}