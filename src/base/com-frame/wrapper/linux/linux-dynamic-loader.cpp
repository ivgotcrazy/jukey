#include <dlfcn.h>

#include <filesystem>

#include "linux-dynamic-loader.h"
#include "component.h"
#include "base-common.h"

using namespace jukey::base;

namespace fs = std::filesystem;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LinuxDynamicLoader::Load(const char* path, const char* name, EntryVec* entries)
{
  if (!path || !name || !entries) {
		BASE_ERR("Invalid parameters!");
		return;
	}

	// Get all dll files
	SharedVec shared_files;
	GetAllSharedFiles(path, &shared_files);

	for (CSTREF shared_file : shared_files) {
		void* module = dlopen(shared_file.c_str(), RTLD_NOW);
		if (module == NULL) {
			BASE_ERR("Failed to load library {}, error: {}", shared_file, dlerror());
			continue;
		}

		void* func = dlsym(module, name);
		if (func) {
      DynamicEntry entry;
      entry.lib_name = shared_file;
      entry.func =  reinterpret_cast<GetComEntryFunc>(func);
      entries->push_back(entry);
      BASE_INF("Find entry in {}", shared_file.c_str());
		} 
    else {
      BASE_ERR("Failed to get entry from {}, error: {}", shared_file, dlerror());
    }

		//dlclose(module);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LinuxDynamicLoader::GetAllSharedFiles(const char* com_path, SharedVec* psv)
{
	if (!psv || !fs::exists(com_path)) {
		BASE_ERR("Invalid param, path: {}", com_path);
		return;
	}

	// file
	if (!fs::is_directory(com_path)) {
		if (fs::path(com_path).extension() == ".so") {
			psv->push_back(com_path);
			BASE_INF("Add shared file {}", com_path);
		}
		return;
	}

	// directory
	fs::directory_iterator begin_iter(com_path);
	fs::directory_iterator end_iter;
	for (auto iter = begin_iter; iter != end_iter; ++iter) {
		if (fs::is_directory(*iter)) {
			GetAllSharedFiles(iter->path().string().c_str(), psv);
		} 
		else {
			if (iter->path().extension() == ".so") {
				psv->push_back(iter->path().string());
				BASE_INF("Add shared file {}", iter->path().string());
			}
		}
			
	}
}

