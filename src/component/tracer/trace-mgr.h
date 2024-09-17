#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <set>

#include "thread/common-thread.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "if-trace-mgr.h"

#include "opentelemetry/trace/tracer.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_options.h"


using namespace opentelemetry;

namespace jukey::com
{

//==============================================================================
// 
//==============================================================================
class TraceMgr 
  : public ITraceMgr
  , public base::ProxyUnknown
  , public base::ComObjTracer
{
public:
  TraceMgr(base::IComFactory* factory, const char* owner);
  ~TraceMgr();

  COMPONENT_FUNCTION_DECL
  COMPONENT_IUNKNOWN_IMPL

  // ITraceMgr
  virtual bool Init(const std::string& url) override;
  virtual SpanId StartSpan(const std::string& span_name) override;
  virtual SpanId StartSpan(const std::string& span_name,
	const std::string& context) override;
  virtual void StopSpan(SpanId span_id) override;
  virtual std::string GetSpanContext(SpanId span_id) override;

private:
  struct TracerEntry
  {
	std::string trace_name;
	nostd::shared_ptr<trace::Tracer> tracer;
  };

  struct SpanEntry
  {
	SpanEntry() {}
	SpanEntry(nostd::shared_ptr<trace::Span> s) : span(s) {}

	nostd::shared_ptr<trace::Span> span;
  };

private:
  exporter::otlp::OtlpHttpExporterOptions m_exporter_opts;
  uint32_t m_next_span_id = 1;
  std::map<SpanId, SpanEntry> m_spans;
  std::mutex m_mutex;
};

} // namespace