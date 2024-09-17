#include <regex>

#include "util-net.h"

namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<com::Address> ParseAddress(const std::string& str)
{
	std::regex addr_reg("(.+):(.+):([0-9]{1,5})");
	std::regex ip_reg("^((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})(\\.((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})){3}$");
	std::regex domain_reg("^([a-zA-Z0-9]([a-zA-Z0-9-_]{0,61}[a-zA-Z0-9])?\\.)+[a-zA-Z]{2,11}$");
	std::cmatch cm;

	if (!std::regex_match(str.c_str(), cm, addr_reg)) {
		return std::nullopt;
	}

	std::optional<com::Address> addr = std::make_optional(com::Address());

	// Protocol
	if (cm[1] == "TCP") {
		addr->type = com::AddrType::TCP;
	}
	else if (cm[1] == "UDP") {
		addr->type = com::AddrType::UDP;
	}
	else {
		return std::nullopt;
	}

	// Host
	if (std::regex_match(cm[2].str(), ip_reg)) {
		addr->ep.host = cm[2];
	}
	else if (std::regex_match(cm[2].str(), domain_reg)) {
		addr->ep.host = cm[2];
	}
	else {
		return std::nullopt;
	}

	// Port
	int port = atoi(cm[3].str().c_str());
	if (port > 0 && port <= 65535) {
		addr->ep.port = (uint16_t)atoi(cm[3].str().c_str());
	}
	else {
		return std::nullopt;
	}

	return addr;
}

}