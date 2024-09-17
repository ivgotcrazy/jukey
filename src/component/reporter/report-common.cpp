#include "report-common.h"

namespace jukey::com
{

void to_json(json& j, const DependEntry& dep)
{
  j = json{
	{"name", dep.name},
	{"desc", dep.desc},
	{"protocol", dep.protocol},
	{"targetType", dep.target_type},
	{"targetId", dep.target_id},
	{"state", dep.state},
  };
}

void from_json(const json& j, DependEntry& dep)
{
  j.at("name").get_to(dep.name);
  j.at("desc").get_to(dep.desc);
  j.at("protocol").get_to(dep.protocol);
  j.at("targetType").get_to(dep.target_type);
  j.at("targetId").get_to(dep.target_id);
  j.at("state").get_to(dep.state);
}

////////////////////////////////////////////////////////////////////////////////

void to_json(json& j, const ReportInfo& info)
{
  j = json{
	{"instance", info.instance},
	{"type", info.service_type},
	{"app", info.app},
	{"space", info.space},
	{"data", info.data},
	{"dependencies", info.depends},
  };
}

void from_json(const json& j, ReportInfo& info)
{
  j.at("instance").get_to(info.instance);
  j.at("type").get_to(info.service_type);
  j.at("app").get_to(info.app);
  j.at("space").get_to(info.space);
  j.at("data").get_to(info.data);
  j.at("dependencies").get_to(info.depends);
}

}