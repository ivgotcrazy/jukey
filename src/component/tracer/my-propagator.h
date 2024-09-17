#pragma once

#include "opentelemetry/context/propagation/text_map_propagator.h"
#include "opentelemetry/trace/span_context.h"

using namespace opentelemetry;
using context::propagation::TextMapCarrier;

namespace jukey::com
{

//==============================================================================
// 
//==============================================================================
class MyPropagator : public context::propagation::TextMapPropagator 
{
public:
  // TextMapPropagator
  virtual context::Context Extract(const TextMapCarrier& carrier,
	context::Context& context) noexcept override;
  virtual void Inject(TextMapCarrier& carrier, 
	const context::Context& context) noexcept override;
  virtual bool Fields(nostd::function_ref<bool(nostd::string_view)> callback) const noexcept override;

private:
  void InjectImpl(TextMapCarrier& carrier, const trace::SpanContext& span_context);
  trace::SpanContext ExtractImpl(const TextMapCarrier& carrier);
};

}