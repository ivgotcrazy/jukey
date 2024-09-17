#include "stream-dumper.h"

namespace jukey::media::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StreamDumper::StreamDumper(const std::string& file_name)
	: DataDumper(file_name)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamDumper::WriteStreamData(const stmr::PinData& data)
{
	for (uint32_t i = 0; i < data.data_count; i++) {
		if (data.media_data[i].data_len > 0) {
			WriteData(DP(data.media_data[i]), data.media_data[i].data_len);
		}
	}
}

}