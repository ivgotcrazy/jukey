#pragma once

#include <inttypes.h>
#include <memory>

#include "component.h"


namespace jukey::com
{

#define CID_TRACE_MGR	"cid-trace-mgr"
#define IID_TRACE_MGR "iid-trace-mgr"

//==============================================================================
// Call chain span
//==============================================================================
typedef uint64_t SpanId;

#define INVALID_SPAN_ID 0

//==============================================================================
// Timer manager
//==============================================================================
class ITraceMgr : public base::IUnknown
{
public:
  //
  // Initialize
  // 
  virtual bool Init(const std::string& url) = 0;

  //
  // Start a root trace span
  // 
  virtual SpanId StartSpan(const std::string& span_name) = 0;

  //
  // Start a trace span with remote context
  //
  virtual SpanId StartSpan(const std::string& span_name, 
	const std::string& context) = 0;

  //
  // Stop a trace span
  //
  virtual void StopSpan(SpanId span_id) = 0;

  //
  // Get span context for propagation
  //
  virtual std::string GetSpanContext(SpanId span_id) = 0;
};

}
