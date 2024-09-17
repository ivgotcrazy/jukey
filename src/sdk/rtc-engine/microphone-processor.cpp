#include "microphone-processor.h"
#include "rtc-common.h"
#include "log.h"
#include "rtc-engine-impl.h"

using namespace jukey::com;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MicrophoneProcessor::MicrophoneProcessor(RtcEngineImpl* engine)
	: m_rtc_engine(engine)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode MicrophoneProcessor::OpenMicrophone(const com::MicParam& param)
{
	LOG_INF("Open microphone, device:{}, chnls:{}, srate:{}, sbits:{}",
		param.dev_id, param.sample_chnl, param.sample_rate, param.sample_bits);

	if (ERR_CODE_OK != G(m_media_engine)->OpenMicrophone(param)) {
		LOG_ERR("Open microphone failed");
		return ERR_CODE_FAILED;
	}

	m_param = param;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode MicrophoneProcessor::CloseMicrophone(uint32_t dev_id)
{
	LOG_INF("Close microphone, device:{}", dev_id);

	ErrCode result = G(m_media_engine)->CloseMicrophone(dev_id);
	if (ERR_CODE_OK != result) {
		LOG_ERR("Close microphone failed");
	}

	result = m_rtc_engine->UnpublishGroupStream(m_stream);
	if (ERR_CODE_OK != result) {
		LOG_ERR("Unpublish group stream failed");
	}

	return result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MicrophoneProcessor::OnAddMicStream(const com::MediaStream& stream)
{
	if (ERR_CODE_OK != m_rtc_engine->PublishGroupStream(stream)) {
		LOG_ERR("Publish group stream failed");
	}

	m_stream = stream;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MicrophoneProcessor::OnDelMicStream(const com::MediaStream& stream)
{
	if (ERR_CODE_OK != m_rtc_engine->UnpublishGroupStream(stream)) {
		LOG_ERR("Publish group stream failed");
	}
}

}