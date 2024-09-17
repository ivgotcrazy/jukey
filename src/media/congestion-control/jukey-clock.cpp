#include "jukey-clock.h"

#include "common/util-time.h"

namespace jukey::cc
{

webrtc::Timestamp JukeyClock::CurrentTime()
{
	return webrtc::Timestamp::Micros(util::Now());
}

webrtc::NtpTime JukeyClock::ConvertTimestampToNtpTime(webrtc::Timestamp timestamp)
{
	return webrtc::NtpTime();
}

}

webrtc::Clock* webrtc::Clock::GetRealTimeClock() {
	static Clock* const clock = new jukey::cc::JukeyClock();
	return clock;
}