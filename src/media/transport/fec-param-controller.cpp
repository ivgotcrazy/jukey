#include "fec-param-controller.h"

#include <cassert>
#include <cmath>
#include <vector>

#include "log.h"
#include "transport-common.h"
#include "common/util-time.h"


namespace jukey::txp
{

////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SimpleFecParamController::SimpleFecParamController(IFecParamHandler* handler, 
	uint8_t k, uint8_t r) : m_handler(handler), m_k(k), m_r(r)
{
	assert(m_k <= 16 && m_r <= 16 && m_k >= m_r);

	m_last_udpate_us = util::Now();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimpleFecParamController::SetMaxProtectionRate(uint32_t rate)
{
	if (rate > 50) {
		m_max_prot_rate = 50;
		LOG_ERR("Invalid protection rate:{}", rate);
	}
	else {
		m_max_prot_rate = rate;
	}

	LOG_INF("Update protection rate:{}", m_max_prot_rate);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimpleFecParamController::TryUpdateFecParam(uint8_t k, uint8_t r)
{
	uint64_t now_us = util::Now();
	
	if (now_us < m_last_udpate_us + kUpdateMinIntervalMs * 1000) {
		return;
	}

	if (k == m_k && r == m_r) {
		m_last_udpate_us = now_us;
		return;
	}

	LOG_INF("Update fec param from {}|{} to {}|{}", m_k, m_r, k, r);

	m_k = k;
	m_r = r;

	m_handler->OnFecParamUpdate(m_k, m_r);
	m_last_udpate_us = now_us;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimpleFecParamController::UpdateStateList(const StateFB& value)
{
	if (m_state_list.size() > kMaxStateSize) {
		m_state_list.pop_front();
	}
	m_state_list.push_back(value);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t SimpleFecParamController::CalcAverRTT()
{
	if (m_state_list.empty()) return 0;

	uint32_t total_rtt = 0;
	for (const auto& state : m_state_list) {
		total_rtt += state.rtt;
	}
	uint32_t aver_rtt = total_rtt / m_state_list.size();

	return aver_rtt;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::pair<uint8_t, uint8_t> SimpleFecParamController::CalcFecParam()
{
	uint32_t max_clc = 0;
	uint32_t total_olr = 0;
	uint32_t total_nlr = 0;

	int i = 3;
	for (auto iter = m_state_list.rbegin(); iter != m_state_list.rend(); ++iter) {
		if (i <= 0) break;

		if (iter->clc > max_clc) {
			max_clc = iter->clc;
		}

		total_olr += iter->olr;
		total_nlr += iter->nlr;

		i--;
	}

	uint32_t aver_olr = (uint32_t)std::ceil((double)total_olr / (3 - i));
	uint32_t aver_nlr = (uint32_t)std::ceil((double)total_nlr / (3 - i));

	// 没有丢包，不需要 FEC 保护
	if (aver_olr == 0) {
		LOG_INF("No loss, no fec protection");
		return std::make_pair(0, 0);
	}

	// 当前 FEC 参数已经保证全部恢复，尽量保持当前参数不变
	if (aver_olr != 0 && aver_nlr == 0 && (m_k != 0 && m_r != 0)) {
		LOG_INF("Keep fec param, k:{}, r:{}", m_k, m_r);
		return std::make_pair(m_k, m_r);
	}

	LOG_INF("max_clc:{}, aver_olr:{}, aver_nlr:{}", max_clc, aver_olr, aver_nlr);

	uint32_t selected_k = 0;
	uint32_t selected_r = 0;

	if (max_clc != 0) {
		if (max_clc > 16) max_clc = 16; // 约束连续丢包保护不超过 16

		for (uint32_t r = max_clc; r >= 1; r--) {
			// 基于连续丢包确定 R 值
			selected_r = r;
			selected_k = 0;

			// 基于原始丢包率确定 K 值
			std::vector<uint32_t> k_list{ 16, 12, 8, 4 };
			for (auto k : k_list) {
				// K 值不能小于 R 值
				if (k < selected_r) break;

				// 计算冗余比率
				uint32_t red_rate = selected_r * 100 / (k + selected_r);

				// 冗余比例不能超过最大保护比例
				if (red_rate > m_max_prot_rate) break;

				selected_k = k;

				// 冗余比率超过了丢包率，退出
				if (red_rate > aver_olr) {
					break;
				}
			}

			if (selected_k != 0) break;
		}

		if (selected_k == 0) {
			selected_r = 0;
		}
	}

	LOG_INF("Calculate fec param, k:{}, r:{}", selected_k, selected_r);

	return std::make_pair(selected_k, selected_r);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimpleFecParamController::OnStateFeedback(const StateFB& value)
{
	UpdateStateList(value);

	if (CalcAverRTT() < kRttThresholdMs) {
		TryUpdateFecParam(0, 0);
	}
	else {
		auto param = CalcFecParam();
		TryUpdateFecParam(param.first, param.second);
	}
}

}