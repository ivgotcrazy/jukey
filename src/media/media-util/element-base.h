#pragma once

#include "common-enum.h"
#include "if-pipeline.h"
#include "if-element.h"
#include "if-pin.h"
#include "log/spdlog-wrapper.h"


namespace jukey::media::util
{

//==============================================================================
// 
//==============================================================================
class ElementBase 
	: public stmr::IElement
	, public stmr::ISinkPinHandler
	, public stmr::ISrcPinHandler
	, public stmr::IPlMsgHandler
{
public:
	ElementBase(base::IComFactory* factory);
	virtual ~ElementBase();

	// IElement
	virtual jukey::com::ErrCode Init(stmr::IPipeline* pipeline, 
		jukey::com::IProperty* props) override;
	virtual jukey::com::ErrCode Start() override;
	virtual jukey::com::ErrCode Pause() override;
	virtual jukey::com::ErrCode Resume() override;
	virtual jukey::com::ErrCode Stop() override;
	virtual stmr::EleState State() override;
	virtual std::string Name() override;
	virtual stmr::EleMainType MainType() override;
	virtual stmr::EleSubType SubType() override;
	virtual stmr::EleMediaType MType() override;
	virtual std::vector<stmr::ISrcPin*> SrcPins() override;
	virtual std::vector<stmr::ISinkPin*> SinkPins() override;
	virtual stmr::IPipeline* Pipeline() override;

	// ISinkPinHandler
	virtual jukey::com::ErrCode OnSinkPinMsg(stmr::ISinkPin* pin,
		const stmr::PinMsg& msg) override;
	virtual jukey::com::ErrCode OnSinkPinData(stmr::ISinkPin* pin,
		const stmr::PinData& data) override;
	virtual jukey::com::ErrCode OnSinkPinNegotiated(stmr::ISinkPin* pin,
		const std::string& cap) override;
	virtual void OnSinkPinConnectState(stmr::ISinkPin* pin, 
		bool connected) override;

	// ISrcPinHandler
	virtual jukey::com::ErrCode OnSrcPinMsg(stmr::ISrcPin* pin,
		const stmr::PinMsg& msg) override;
	virtual void OnSrcPinConnectState(stmr::ISrcPin* src_pin, bool add) override;
	virtual jukey::com::ErrCode OnSrcPinNegotiated(stmr::ISrcPin* src_pin,
		const std::string& cap) override;

	// IPlMsgHandler
	virtual void OnPipelineMsg(const jukey::com::CommonMsg& msg) override;

protected: // element overwrite
	virtual jukey::com::ErrCode DoInit(jukey::com::IProperty* props);
	virtual jukey::com::ErrCode DoStart();
	virtual jukey::com::ErrCode DoPause();
	virtual jukey::com::ErrCode DoResume();
	virtual jukey::com::ErrCode DoStop();

	virtual jukey::com::ErrCode ProcSinkPinMsg(stmr::ISinkPin* pin,
		const stmr::PinMsg& msg);
	virtual jukey::com::ErrCode ProcSrcPinMsg(stmr::ISrcPin* pin,
		const stmr::PinMsg& msg);
	virtual jukey::com::ErrCode PreProcPipelineMsg(
		const jukey::com::CommonMsg& msg);

	void NotifyRunState(const std::string& desc);
	bool ShouldMakeNegotiate();

private:
	jukey::com::ErrCode OnPinStartMsg(stmr::ISinkPin* pin,
		const stmr::PinMsg& msg);
	jukey::com::ErrCode OnPinPauseMsg(stmr::ISinkPin* pin,
		const stmr::PinMsg& msg);
	jukey::com::ErrCode OnPinResumeMsg(stmr::ISinkPin* pin,
		const stmr::PinMsg& msg);
	jukey::com::ErrCode OnPinStopMsg(stmr::ISinkPin* pin,
		const stmr::PinMsg& msg);

	jukey::com::ErrCode TransferSrcPinMsg(const stmr::PinMsg& msg);
	jukey::com::ErrCode TransferSinkPinMsg(const stmr::PinMsg& msg);

	jukey::com::ErrCode DoStartElement();
	jukey::com::ErrCode DoPauseElement();
	jukey::com::ErrCode DoResumeElement();
	jukey::com::ErrCode DoStopElement();

	void OnStartElementMsg(const jukey::com::CommonMsg& msg);
	void OnStopElementMsg(const jukey::com::CommonMsg& msg);
	void OnPauseElementMsg(const jukey::com::CommonMsg& msg);
	void OnResumeElementMsg(const jukey::com::CommonMsg& msg);

protected:
	stmr::IPipeline* m_pipeline = nullptr;
	base::IComFactory* m_factory = nullptr;

	// Element attributes
	std::string  m_ele_name   = "";
	stmr::EleSubType   m_sub_type   = stmr::EleSubType::INVALID;
	stmr::EleState     m_ele_state  = stmr::EleState::INVALID;
	stmr::EleMainType  m_main_type  = stmr::EleMainType::INVALID;
	stmr::EleMediaType m_media_type = stmr::EleMediaType::INVALID;

	std::vector<stmr::ISrcPin*>  m_src_pins;
	std::vector<stmr::ISinkPin*> m_sink_pins;

	bool m_pins_created = true;

	jukey::util::SpdlogWrapperSP m_logger;
};

}