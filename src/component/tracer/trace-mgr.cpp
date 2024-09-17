#include "trace-mgr.h"
#include "log.h"
#include "common-struct.h"
#include "common/util-time.h"
#include "my-propagator.h"
#include "my-carrier.h"

#include "opentelemetry/exporters/otlp/otlp_http_exporter_factory.h"
#include "opentelemetry/sdk/common/global_log_handler.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/sdk/version/version.h"
#include "opentelemetry/context/propagation/text_map_propagator.h"
#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/context/context_value.h"
#include "opentelemetry/context/runtime_context.h"


namespace nostd = opentelemetry::nostd;
namespace trace = opentelemetry::trace;
namespace context = opentelemetry::context;
namespace propagation = opentelemetry::context::propagation;
namespace sdk_trace = opentelemetry::sdk::trace;

using opentelemetry::context::propagation::TextMapPropagator;

using namespace jukey::base;

typedef nostd::shared_ptr<trace::Tracer> TracerNSP;
typedef nostd::shared_ptr<trace::TracerProvider> TracerProviderNSP;
typedef std::shared_ptr<trace::TracerProvider> TracerProviderSP;

namespace 
{

TracerNSP GetTracer(const std::string& tracer_name)
{
  auto provider = trace::Provider::GetTracerProvider();
  return provider->GetTracer(tracer_name);
}

}

namespace jukey::com
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TraceMgr::TraceMgr(base::IComFactory* factory, const char* owner)
  : base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_TRACE_MGR, owner)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TraceMgr::~TraceMgr()
{
  TracerProviderNSP provider = trace::Provider::GetTracerProvider();
  static_cast<sdk_trace::TracerProvider*>(provider.get())->ForceFlush();

  std::shared_ptr<trace::TracerProvider> none;
  trace::Provider::SetTracerProvider(none);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* TraceMgr::CreateInstance(IComFactory* factory,
	const char* cid, const char* owner)
{
  if (strcmp(cid, CID_TRACE_MGR) == 0) {
	return new TraceMgr(factory, owner);
  }
  else {
	return nullptr;
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* TraceMgr::NDQueryInterface(const char* riid)
{
  if (0 == strcmp(riid, IID_TRACE_MGR)) {
	return static_cast<ITraceMgr*>(this);
  }
  else {
	return ProxyUnknown::NDQueryInterface(riid);
  }
}


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TraceMgr::Init(const std::string& url)
{
  m_exporter_opts.url = url;

  LOG_INF("Set exporter url:{}", url);

  auto exporter
	= exporter::otlp::OtlpHttpExporterFactory::Create(m_exporter_opts);
  if (!exporter) {
	LOG_ERR("Create otlp http exporter failed!");
	return false;
  }

  auto processor
	= sdk_trace::SimpleSpanProcessorFactory::Create(std::move(exporter));
  if (!processor) {
	LOG_ERR("Create simple span processor failed!");
	return false;
  }

  TracerProviderSP provider
	= sdk_trace::TracerProviderFactory::Create(std::move(processor));
  if (!provider) {
	LOG_ERR("Create tracer provider failed!");
	return false;
  }

  trace::Provider::SetTracerProvider(provider);

  // Set user-defined propagator
  propagation::GlobalTextMapPropagator::SetGlobalPropagator(
	nostd::shared_ptr<TextMapPropagator>(new MyPropagator()));

  return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SpanId TraceMgr::StartSpan(const std::string& span_name)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  TracerNSP tracer = GetTracer("test");
  if (!tracer) {
	LOG_ERR("Invalid tracer!");
	return INVALID_SPAN_ID;
  }

  nostd::shared_ptr<trace::Span> span = tracer->StartSpan(span_name);
  if (!span) {
	LOG_ERR("Start span:{} failed!", span_name);
	return INVALID_SPAN_ID;
  }

  m_spans.insert(std::make_pair(m_next_span_id, SpanEntry(span)));

  LOG_INF("Insert span:{}, total size:{}", m_next_span_id, m_spans.size());

  return m_next_span_id++;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SpanId TraceMgr::StartSpan(const std::string& span_name,
  const std::string& context)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto prop = propagation::GlobalTextMapPropagator::GetGlobalPropagator();
  auto cur_ctx = context::RuntimeContext::GetCurrent();
  auto new_ctx = prop->Extract(MyCarrier(context), cur_ctx);

  TracerNSP tracer = GetTracer("test");
  if (!tracer) {
	LOG_ERR("Invalid tracer!");
	return INVALID_SPAN_ID;
  }

  trace::StartSpanOptions opts;
  opts.parent = trace::GetSpan(new_ctx)->GetContext();

  SpanEntry entry(tracer->StartSpan(span_name, {}, opts));
  m_spans.insert(std::make_pair(m_next_span_id, entry));

  LOG_INF("Insert span:{}, total size:{}", m_next_span_id, m_spans.size());
  
  return m_next_span_id++;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TraceMgr::StopSpan(SpanId span_id)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto iter = m_spans.find(span_id);
  if (iter == m_spans.end()) {
	LOG_ERR("Cannot find span by span id:{}", span_id);
	return;
  }

  LOG_INF("Insert span:{}, total size:{}", iter->first, m_spans.size());

  iter->second.span->End();
  m_spans.erase(iter);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string TraceMgr::GetSpanContext(SpanId span_id)
{
  auto iter = m_spans.find(span_id);
  if (iter == m_spans.end()) {
	LOG_ERR("Cannot find span by id:{}", span_id);
	return std::string();
  }

  auto prop = propagation::GlobalTextMapPropagator::GetGlobalPropagator();

  GetTracer("test")->WithActiveSpan(iter->second.span);

  auto cur_ctx = context::RuntimeContext::GetCurrent();

  MyCarrier carrier;
  prop->Inject(carrier, cur_ctx);

  return carrier.ToString();
}

}
