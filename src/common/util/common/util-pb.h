#pragma once

#include <string>

#include "common-struct.h"
#include "protoc/common.pb.h"

#include "google/protobuf/message.h"

namespace jukey::util
{

// Convert protobuf data to json string
std::string PbMsgToJson(const google::protobuf::Message& message);

// Convert prot::NetStream to com::NetStream
com::MediaStream ToMediaStream(const prot::NetStream& stream);

}
