#pragma once

#include <vector>

#include "if-dynamic-loader.h"

namespace jukey::base
{

//==============================================================================
// Windows version
//==============================================================================
class WinDynamicLoader : public IDynamicLoader
{
public:
	// IDynamicLoader
	void Load(const std::string& path, const std::string& name, 
		EntryVec* entries) override;

private:
	typedef std::vector<std::string> DllVec;
	void GetAllDllFiles(const std::string& com_path, DllVec* pdv);
};

}

