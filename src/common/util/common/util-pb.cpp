#include "util-common.h"

#include "util-pb.h"
#include "google/protobuf/util/json_util.h"

namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string PbMsgToJson(const google::protobuf::Message& message)
{
	std::string json_data;
	google::protobuf::util::MessageToJsonString(message, &json_data);
	return json_data;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::MediaStream ToMediaStream(const prot::NetStream& stream)
{
	com::MediaStream media_stream;

	MAPP_ID(media_stream)   = stream.app_id();
	MUSER_ID(media_stream)  = stream.user_id();
	STRM_ID(media_stream)   = stream.stream_id();
	STRM_TYPE(media_stream) = (com::StreamType)stream.stream_type();
	MSRC_ID(media_stream)   = stream.media_src_id();
	MSRC_TYPE(media_stream) = (com::MediaSrcType)stream.media_src_type();

	return media_stream;
}

}
