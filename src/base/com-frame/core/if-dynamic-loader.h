#pragma once

#include <string>
#include <vector>
#include "component.h"

namespace jukey::base
{

//==============================================================================
// Loading dynamic link libraries that contain components
//==============================================================================
class IDynamicLoader
{
public:
	struct DynamicEntry
	{
		std::string lib_name;
		GetComEntryFunc func = nullptr;
	};

	// Dynamic link entries
	typedef std::vector<DynamicEntry> EntryVec;

	/**
	 * @brief Load dynamic link libraries 
	 */
	virtual void Load(const std::string& path, const std::string& name,
		EntryVec* entries) = 0;
};

}
