#ifndef JINJA2CPP_SRC_HOST_CALL_CONTEXT_H
#define JINJA2CPP_SRC_HOST_CALL_CONTEXT_H

#include <jinja2cpp/value.h>

// The context a host's callable gets (UserCallableParams::context) and the error form of a lent template callable.
// Kept out of internal_value.cpp so that the host-call path does not grow the render path's unit: GCC's inlining
// budget is per unit, and the growth moved hot string constructions out of line (#813, perf track)

namespace jinja2
{
inline namespace v2
{
class InternalValue;
class RenderContext;

namespace host_call
{
// How a host's call gets a value of the template: lent for the call as its arguments are (ArgumentToHost); `scope` is
// the call's HostCallScope
using LendFn = Value (*)(const InternalValue& value, RenderContext& context, void* scope);

struct ContextMap
{
    GenericMap map;
    // Expired with EndContextMap when the call ends
    const IMapItemAccessor* handle = nullptr;
};

// The render's lookup as a map, lending what it gives through `lend`. The map of the call that ended last on this
// thread is used again when the host kept no copy of it (EndContextMap), which saves the call an allocation
ContextMap MakeContextMap(RenderContext& context, void* scope, LendFn lend);

// The host's call the context was given to ended: a copy the host kept fails with TemplateExpired. `context` is the
// call's own copy (UserCallableParams::context), which goes with the call: the map is kept for the next call on this
// thread, which uses it again only if the host kept no copy
void EndContextMap(const IMapItemAccessor* handle, Value& context) noexcept;

// Called in a catch handler: rethrows the current exception as the host sees it, the public ErrorInfo for the
// engine's errors (a located RenderFailure keeps its code) and a host's own exception as it is
[[noreturn]] void RethrowAsPublicError();
} // namespace host_call
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_HOST_CALL_CONTEXT_H
