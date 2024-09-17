#pragma once

#include "system_wrappers/include/clock.h"

namespace jukey::cc
{

class JukeyClock : public webrtc::Clock
{
public:
	virtual webrtc::Timestamp CurrentTime() override;
	virtual webrtc::NtpTime ConvertTimestampToNtpTime(webrtc::Timestamp timestamp) override;
};

}