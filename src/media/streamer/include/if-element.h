#pragma once

#include <inttypes.h>
#include <list>
#include <string>

#include "component.h"
#include "common-enum.h"
#include "common-error.h"
#include "if-property.h"
#include "com-factory.h"


namespace jukey::stmr
{

//==============================================================================
// Element cid define
//==============================================================================
#define CID_AUDIO_CONVERT  "cid-audio-convert-element"
#define CID_AUDIO_DECODE   "cid-audio-decode-element"
#define CID_AUDIO_ENCODE   "cid-audio-encode-element"
#define CID_AUDIO_MIX      "cid-audio-mix-element"
#define CID_AUDIO_PLAY     "cid-audio-play-element"
#define CID_AUDIO_PROXY    "cid-audio-proxy-element"
#define CID_AUDIO_RECV     "cid-audio-recv-element"
#define CID_AUDIO_SEND     "cid-audio-send-element"
#define CID_AUDIO_TEST     "cid-audio-test-element"
#define CID_CAMERA         "cid-camera-element"
#define CID_FILE_DEMUX     "cid-file-demux-element"
#define CID_MICROPHONE     "cid-microphone-element"
#define CID_RECORD         "cid-record-element"
#define CID_VIDEO_CONVERT  "cid-video-convert-element"
#define CID_VIDEO_DECODE   "cid-video-decode-element"
#define CID_VIDEO_ENCODE   "cid-video-encode-element"
#define CID_VIDEO_MIX      "cid-video-mix-element"
#define CID_VIDEO_PROXY    "cid-video-proxy-element"
#define CID_VIDEO_RECV     "cid-video-recv-element"
#define CID_VIDEO_RENDER   "cid-video-render-element"
#define CID_VIDEO_SEND     "cid-video-send-element"

//==============================================================================
// Element iid define
//==============================================================================
#define IID_ELEMENT "iid-element"

//==============================================================================
// Main type of element
//==============================================================================
enum class EleMainType
{
	INVALID = 0,

	SRC     = 1,
	SINK    = 2,
	FILTER  = 3,
};


//==============================================================================
// Sub type of element
//==============================================================================
enum class EleSubType
{
	INVALID = 0,

	// SRC
	DEMUXER,
	CAPTURER,
	PROXY,
	RECEIVER,

	// FILTER
	DECODER,
	ENCODER,
	CONVERTER,
	MIXER,

	// SINK
	MUXER,
	PLAYER,
	SENDER,
	TESTER,
};

//==============================================================================
// Which media type to process
//==============================================================================
enum class EleMediaType
{
	INVALID     = 0,

	AUDIO_ONLY  = 1,
	VIDEO_ONLY  = 2,
	AUDIO_VIDEO = 3,
};

//==============================================================================
//        INVALID
//           |
//           |init
//           ↓
//  |-----INITED
//  |        |
//  |stop    |start(resume)
//  |        ↓ 
//  |-----RUNNING<----|
//  |        |        |
//  |        |pause   |resume
//  |        ↓        |
//  |stop PAUSED------|
//  |        |
//  |        |stop
//  |        ↓
//  |----->STOPED
//==============================================================================
enum class EleState
{
	INVALID = 0,
	INITED  = 1,
	PAUSED  = 2,
	RUNNING = 3,
	STOPED  = 4,
};

class ISrcPin;
class ISinkPin;
class IPipeline;
//==============================================================================
// An element can have at most one Source Pin and one Sink Pin
//==============================================================================
class IElement : public base::IUnknown
{
public:
	//
	// @brief Initialize element
	// @param pipeline element belongs to
	// @param param    paramenters in property format
	//
	virtual com::ErrCode Init(IPipeline* pipeline, com::IProperty* param) = 0;

	//
	// @brief Start element
	//
	virtual com::ErrCode Start() = 0;

	//
	// @brief Pause element
	//
	virtual com::ErrCode Pause() = 0;

	//
	// @brief resume element
	//
	virtual com::ErrCode Resume() = 0;

	//
	// @brief Stop element
	//
	virtual com::ErrCode Stop() = 0;

	//
	// @brief Get element name
	//
	virtual std::string Name() = 0;

	//
	// @brief Get element state
	//
	virtual EleState State() = 0;

	//
	// @brief Get element main type
	//
	virtual EleMainType MainType() = 0;

	//
	// @brief Get element sub type
	//
	virtual EleSubType SubType() = 0;

	//
	// @brief Get element media type
	//
	virtual EleMediaType MType() = 0;

	/**
	 * @brief Get src pin by pin name
	 */
	virtual std::vector<ISrcPin*> SrcPins() = 0;

	//
	// Get sink pin by pin name
	//
	virtual std::vector<ISinkPin*> SinkPins() = 0;

	//
	// Get pipeline
	//
	virtual IPipeline* Pipeline() = 0;
};

}