#include <windows.h>
#include <string>
#include <filesystem>

#include "win-dynamic-loader.h"
#include "component.h"
#include "log.h"


namespace fs = std::filesystem;

namespace jukey::base
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void WinDynamicLoader::Load(const std::string& path, const std::string& name,
	EntryVec* entries)
{
	if (!entries) {
		LOG_ERR("Invalid parameters!");
		return;
	}

	// Get all dll files
	DllVec dll_files;
	GetAllDllFiles(path, &dll_files);

	for (CSTREF dll_file : dll_files) {
		HMODULE module = ::LoadLibraryA(dll_file.c_str());
		if (module == NULL) {
			LOG_ERR("Failed to load library {}, error: {}", dll_file,
				::GetLastError());
			continue;
		}

		void* func = ::GetProcAddress(module, name.c_str());
		if (!func) {
			LOG_WRN("Failed to get component from {}, error: {}", dll_file,
				::GetLastError());
			::FreeLibrary(module);
			continue;
		}

		DynamicEntry entry;
		entry.lib_name = dll_file;
		entry.func = static_cast<GetComEntryFunc>(func);
		entries->push_back(entry);

		LOG_INF("Find entry in {}", dll_file.c_str());
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void WinDynamicLoader::GetAllDllFiles(const std::string& com_path, DllVec* pdv)
{
	if (!pdv || !fs::exists(com_path)) {
		LOG_ERR("Invalid param, path: {}", com_path);
		return;
	}

	// file
	if (!fs::is_directory(com_path)) {
		if (fs::path(com_path).extension() == ".dll") {
			pdv->push_back(com_path);
			LOG_INF("Add dll file {}", com_path);
		}
		return;
	}

	// directory
	fs::directory_iterator begin_iter(com_path);
	fs::directory_iterator end_iter;
	for (auto iter = begin_iter; iter != end_iter; ++iter) {
		if (fs::is_directory(*iter)) {
			GetAllDllFiles(iter->path().string().c_str(), pdv);
		}
		else {
			if (iter->path().extension() == ".dll") {
				pdv->push_back(iter->path().string());
				LOG_INF("Add dll file {}", iter->path().string());
			}
		}
	}
}

}