#pragma once

#include <string>
#include <map>

#include "nlohmann/json.hpp"

#include "include/if-reporter.h"

using json = nlohmann::json;

namespace jukey::com
{

void to_json(json& j, const DependEntry& dep);
void from_json(const json& j, DependEntry& dep);

////////////////////////////////////////////////////////////////////////////////

struct ReportInfo
{
	std::string instance;
	std::string service_type;
	std::string app;
	std::string space;
	std::string data;

	std::vector<DependEntry> depends;
};

void to_json(json& j, const ReportInfo& info);
void from_json(const json& j, ReportInfo& info);

}

