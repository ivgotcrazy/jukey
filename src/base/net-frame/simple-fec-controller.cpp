#include "simple-fec-controller.h"
#include "common/util-time.h"
#include "log.h"


using namespace jukey::util;

#define LOSS_THRESHOLD 10 // 1%
#define TIME_THRESHOLD 10000000 // 10s

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimpleFecController::UpdateLossInfo(const LossInfo& info)
{
	while (m_loss_info_count >= s_max_info_count) {
		m_total_fec_loss -= m_loss_infos.front().fec_loss;
		m_total_ses_loss -= m_loss_infos.front().ses_loss;

		m_loss_infos.pop_front();
		m_loss_info_count--;
	}

	m_loss_infos.push_back(info);
	m_loss_info_count++;

	m_total_fec_loss += info.fec_loss;
	m_total_ses_loss += info.ses_loss;

	m_aver_fec_loss = m_total_fec_loss / m_loss_info_count;
	m_aver_ses_loss = m_total_ses_loss / m_loss_info_count;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
FecParam SimpleFecController::GetFecParam()
{
	if (m_aver_fec_loss == 0) { // no fec loss
		if (m_fec_param_index == 0) { // fec is closed
			if (m_aver_ses_loss > LOSS_THRESHOLD) { // session loss (1%)
				m_fec_param_index = 1; // open fec
				m_last_fec_param_ts = util::Now();
				LOG_INF("Open fec: [{},{}]", m_fec_param_table[m_fec_param_index].k,
					m_fec_param_table[m_fec_param_index].r);
			}
		}
		else { // fec is opened
			if (m_aver_ses_loss > 0) {
				if (m_aver_ses_loss > LOSS_THRESHOLD) { // wired!!!
					LOG_ERR("FEC loss is zero, but with session loss!");
				}
			}
			else { // no fec loss and no session loss,
				if (m_last_fec_param_ts + TIME_THRESHOLD <= Now()) { // wait 10s
					LOG_INF("Close fec: [{},{}]", m_fec_param_table[m_fec_param_index].k,
						m_fec_param_table[m_fec_param_index].r);
					m_fec_param_index = 0; // close fec
					m_last_fec_param_ts = util::Now();
				}
			}
		}
	}
	else { // fec loss (fec must be opened)
		if (m_fec_param_index == 0) { // wired!!!
			LOG_ERR("Come fec loss but fec is closed!");
		}
		else { // fec is opened
			if (m_aver_ses_loss > LOSS_THRESHOLD) { // session loss
				if (m_last_fec_param_ts + TIME_THRESHOLD <= Now()) { // wait 10s
					if (m_fec_param_index < s_max_param_size - 1) {
						m_fec_param_index++; // improve param
						m_last_fec_param_ts = util::Now();
						LOG_INF("Improve fec: [{},{}]", m_fec_param_table[m_fec_param_index].k,
							m_fec_param_table[m_fec_param_index].r);
					}
				}
			}
		}
	}

	return m_fec_param_table[m_fec_param_index];
}

}