#include "util-string.h"

namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string MEDIA_SRC_TYPE_STR(com::MediaSrcType src_type)
{
	switch (src_type) {
	case com::MediaSrcType::INVALID:
		return "invalid";
	case com::MediaSrcType::FILE:
		return "file";
	case com::MediaSrcType::RTSP:
		return "rtsp";
	case com::MediaSrcType::MICROPHONE:
		return "microphone";
	case com::MediaSrcType::CAMERA:
		return "camera";
	default:
		return "unknown";
	}
}

}