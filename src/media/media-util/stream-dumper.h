#pragma once

#include <memory>

#include "common/util-dump.h"
#include "if-pin.h"

namespace jukey::media::util
{

//==============================================================================
// 
//==============================================================================
class StreamDumper : public jukey::util::DataDumper
{
public:
	StreamDumper(const std::string& file_name);

	void WriteStreamData(const stmr::PinData& data);
};
typedef std::shared_ptr<StreamDumper> StreamDumperSP;

}