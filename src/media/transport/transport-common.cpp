#include "transport-common.h"

namespace jukey::txp
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StateFeedback ToStateFeedback(const StateFB& fb)
{
		StateFeedback feedback;

		feedback.start_time = fb.start_time;
		feedback.end_time = fb.end_time;
		feedback.recv_count = fb.recv_count;
		feedback.lost_count = fb.lost_count;
		feedback.rtt = fb.rtt;
		feedback.olr = fb.olr;
		feedback.flr = fb.flr;
		feedback.nlr = fb.nlr;
		feedback.clc = fb.clc;
		feedback.flc = fb.flc;
		feedback.fc = fb.fc;

		return feedback;
}

}