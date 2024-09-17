#pragma once

#include <list>
#include "if-fec-controller.h"

namespace jukey::net
{

//==============================================================================
// Not thread safe
//==============================================================================
class SimpleFecController : public IFecController
{
public:
	// IFecController
	virtual void UpdateLossInfo(const LossInfo& info) override;
	virtual FecParam GetFecParam() override;

private:
	std::list<LossInfo> m_loss_infos;
	uint32_t m_loss_info_count = 0; // for performance

	uint32_t m_total_ses_loss = 0;
	uint32_t m_total_fec_loss = 0;

	uint32_t m_aver_ses_loss = 0;
	uint32_t m_aver_fec_loss = 0;

	const static uint32_t s_max_info_count = 100;
	const static uint32_t s_max_param_size = 6;

	FecParam m_fec_param_table[s_max_param_size] = {
		{0, 0}, {5, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 5}
	};
	
	uint32_t m_fec_param_index = 0;
	uint64_t m_last_fec_param_ts = 0;
};

}