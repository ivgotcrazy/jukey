#pragma once

#include <vector>
#include <string>

#include "if-dynamic-loader.h"

namespace jukey
{
namespace base
{

//==============================================================================
// Linux version
//==============================================================================
class LinuxDynamicLoader : public IDynamicLoader
{
public:
	void Load(const char* path, const char* name, EntryVec* entries) override;

private:
	typedef std::vector<std::string> SharedVec;
	void GetAllSharedFiles(const char* com_path, SharedVec* psv);
};

}
}
