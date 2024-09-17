#pragma once

#include <memory>
#include "common-struct.h"

namespace jukey::txp
{

//==============================================================================
// 
//==============================================================================
class IFramePackHandler
{
public:
	virtual void OnSegmentData(const com::Buffer& buf) = 0;
};

//==============================================================================
// 
//==============================================================================
class FramePacker
{
public:
	FramePacker(com::StreamType st, IFramePackHandler* handler);
	~FramePacker();

	void WriteFrameData(const com::Buffer& buf);

private:
	IFramePackHandler* m_handler = nullptr;
	com::StreamType m_st = com::StreamType::INVALID;
};
typedef std::unique_ptr<FramePacker> FramePackerUP;

}