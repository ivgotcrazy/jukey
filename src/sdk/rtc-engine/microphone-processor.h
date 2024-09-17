#pragma once

#include "common-struct.h"
#include "common-error.h"

namespace jukey::sdk
{

class RtcEngineImpl;

//==============================================================================
// 
//==============================================================================
class MicrophoneProcessor
{
public:
	MicrophoneProcessor(RtcEngineImpl* engine);

	com::ErrCode OpenMicrophone(const com::MicParam& param);
	com::ErrCode CloseMicrophone(uint32_t dev_id);

	void OnAddMicStream(const com::MediaStream& stream);
	void OnDelMicStream(const com::MediaStream& stream);

private:
	RtcEngineImpl* m_rtc_engine = nullptr;
	com::MicParam m_param;
	com::MediaStream m_stream;
};

}