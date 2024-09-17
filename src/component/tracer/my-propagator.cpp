#include "my-propagator.h"
#include "opentelemetry/context/runtime_context.h"
#include "opentelemetry/trace/context.h"
#include "opentelemetry/trace/propagation/detail/hex.h"


static const size_t kTraceIdSize = 32;
static const size_t kSpanIdSize = 16;
static const size_t kTraceFlagSize = 2;

namespace jukey::com
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
static trace::TraceId TraceIdFromHex(nostd::string_view trace_id)
{
	uint8_t buf[kTraceIdSize / 2];
	trace::propagation::detail::HexToBinary(trace_id, buf, sizeof(buf));
	return trace::TraceId(buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
static trace::SpanId SpanIdFromHex(nostd::string_view span_id)
{
	uint8_t buf[kSpanIdSize / 2];
	trace::propagation::detail::HexToBinary(span_id, buf, sizeof(buf));
	return trace::SpanId(buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
trace::SpanContext MyPropagator::ExtractImpl(const TextMapCarrier& carrier)
{
	nostd::string_view trace_id = carrier.Get("trace-id");
	nostd::string_view span_id = carrier.Get("span-id");
	nostd::string_view trace_flags = carrier.Get("trace-flags");
	nostd::string_view trace_state = carrier.Get("trace-state");


	return trace::SpanContext(TraceIdFromHex(trace_id),
		SpanIdFromHex(span_id),
		trace::TraceFlags(0),
		true,
		trace::TraceState::FromHeader(trace_state));
}

//------------------------------------------------------------------------------
// Extract carrier to context
//------------------------------------------------------------------------------
context::Context MyPropagator::Extract(const TextMapCarrier& carrier,
	context::Context& context) noexcept
{
	trace::SpanContext span_context = ExtractImpl(carrier);
	nostd::shared_ptr<trace::Span> sp{ new trace::DefaultSpan(span_context) };
	return trace::SetSpan(context, sp);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MyPropagator::InjectImpl(TextMapCarrier& carrier,
	const trace::SpanContext& span_context)
{
	char trace_id[kTraceIdSize] = { 0 };
	span_context.trace_id().ToLowerBase16(
		nostd::span<char, 2 * trace::TraceId::kSize>{trace_id, kTraceIdSize});

	char span_id[kSpanIdSize] = { 0 };
	span_context.span_id().ToLowerBase16(
		nostd::span<char, 2 * trace::SpanId::kSize>{span_id, kSpanIdSize});

	char trace_flags[kTraceFlagSize] = { 0 };
	span_context.trace_flags().ToLowerBase16(
		nostd::span<char, 2 * 1>{trace_flags, kTraceFlagSize});

	carrier.Set("trace-id", nostd::string_view(trace_id, sizeof(trace_id)));
	carrier.Set("span-id", nostd::string_view(span_id, sizeof(span_id)));
	carrier.Set("trace-flags", nostd::string_view(trace_flags, sizeof(trace_flags)));
	carrier.Set("trace-state", span_context.trace_state()->ToHeader());
}

//------------------------------------------------------------------------------
// Inject context to carrier
//------------------------------------------------------------------------------
void MyPropagator::Inject(TextMapCarrier& carrier,
	const context::Context& context) noexcept
{
	trace::SpanContext span_context = trace::GetSpan(context)->GetContext();
	if (span_context.IsValid()) {
		InjectImpl(carrier, span_context);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MyPropagator::Fields(nostd::function_ref<bool(nostd::string_view)> callback) const noexcept
{
	return true;
}

}