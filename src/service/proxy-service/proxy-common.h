#pragma once

#include <optional>

#include "common-struct.h"
#include "net-public.h"

namespace jukey::srv
{

#define APP_CHECK_AND_RETURN(header) \
  if (header->app == 0) {            \
    LOG_ERR("Invalid app ID!");      \
    return;                          \
  }

#define CLT_CHECK_AND_RETURN(header) \
  if (header->clt == 0) {            \
    LOG_ERR("Invalid cleint ID!");   \
    return;                          \
  }

#define USR_CHECK_AND_RETURN(header) \
  if (header->usr == 0) {            \
    LOG_ERR("Invalid user ID!");     \
    return;                          \
  }

#define GRP_CHECK_AND_RETURN(header) \
  if (header->grp == 0) {            \
    LOG_ERR("Invalid group ID!");    \
    return;                          \
  }

struct RouteEntry
{
	net::SessionId sid = 0;
	bool normal = false;
};

typedef RouteEntry CltRouteEntry;

struct UsrRouteEntry
{
	net::SessionId sid = 0;
	bool normal = false;
	uint32_t client_id = 0;
};

// value:normal
typedef std::unordered_map<net::SessionId, bool> GrpRouteEntry;

typedef std::optional<CltRouteEntry> CltRouteEntryOpt;
typedef std::optional<UsrRouteEntry> UsrRouteEntryOpt;
typedef std::optional<GrpRouteEntry> GrpRouteEntryOpt;

}