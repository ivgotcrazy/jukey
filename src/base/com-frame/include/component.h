#pragma once

#include <inttypes.h>
#include "if-unknown.h"
#include "common-export.h"

namespace jukey::base
{

class IComFactory;

// Prototype for creating component
typedef IUnknown* (*CreateComFunc)(IComFactory*, const char*/*cid*/, const char*/*owner*/);

//==============================================================================
// Component entry
//==============================================================================
struct ComEntry
{
	const char* com_name;
	const char* com_cid;
	CreateComFunc com_create;
};

// Dynamic link library exporting function name
#define COMPONENT_ENTRY_NAME "GetComEntry"

// Prototype of exporting function
typedef void (*GetComEntryFunc)(ComEntry** entries, uint32_t* count);

// Implementation of component entry function
#define COMPONENT_ENTRY_IMPLEMENTATION                                         \
  extern "C"                                                                   \
  {                                                                            \
    JUKEY_API void JUKEY_CALL GetComEntry(ComEntry** entries, uint32_t* count) \
    {                                                                          \
      *entries = com_entries;                                                  \
      *count = static_cast<uint32_t>(sizeof(com_entries) / sizeof(ComEntry));  \
    }                                                                          \
  }

}