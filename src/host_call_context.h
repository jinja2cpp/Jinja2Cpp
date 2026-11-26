#ifndef JINJA2CPP_SRC_HOST_CALL_CONTEXT_H
#define JINJA2CPP_SRC_HOST_CALL_CONTEXT_H

#include "node_arena.h"
#include "slot_frame.h"

#include <jinja2cpp/value.h>

// The context a host's callable gets (UserCallableParams::context) and the error form of a lent template callable.
// Kept out of internal_value.cpp so that the host-call path does not grow the render path's unit: GCC's inlining
// budget is per unit, and the growth moved hot string constructions out of line (#813, perf track)

namespace jinja2
{
inline namespace v2
{
struct CallParams;
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

// A call expression in a loop or block body that has `set`s before it, while it calls an expression callable
// (docs/tasks/0558). `params` is how the host's callable finds it: a filter, a test or a call made from elsewhere builds
// parameters of its own
struct CallSite
{
    const CallParams* params = nullptr;
    ArenaSpan<SlotName> bodySets;
    const CallSite* outer = nullptr;
};

// The render's lookup as a map, lending what it gives through `lend`. The map of the call that ended last on this
// thread is used again when the host kept no copy of it (EndContextMap), which saves the call an allocation
// A host that keeps only the map's address (GetAccessor, GetIdentity), without a copy, may find it naming a later call's map
//
// The call a template made in the body of a loop or block, with the names `set` stored before it there (CallSite), gives
// the map the names that are ContextNames::BodyVars
ContextMap MakeContextMap(RenderContext& context, void* scope, LendFn lend, const CallSite* site);

// Makes `site` the thread's call site for its scope, and the one before it again when the scope ends. Out of line, with
// the thread-local: no extern-TLS wrapper and no growth of the render's unit
class CallSiteScope
{
public:
    explicit CallSiteScope(CallSite& site) noexcept;
    ~CallSiteScope();
    CallSiteScope(const CallSiteScope&) = delete;
    CallSiteScope(CallSiteScope&&) = delete;
    CallSiteScope& operator=(const CallSiteScope&) = delete;
    CallSiteScope& operator=(CallSiteScope&&) = delete;

private:
    const CallSite& m_site;
};

// The site of the call whose parameters are `params`, or null
const CallSite* SiteOf(const CallParams& params) noexcept;
// The thread's innermost site, for the checks of a suspendable render (SuspensionMarks)
const void* CurrentSite() noexcept;

// The host's call the context was given to ended: a copy the host kept fails with TemplateExpired. `context` is the
// call's own copy (UserCallableParams::context), which goes with the call: the map is kept for the next call on this
// thread, which uses it again only if the host kept no copy
void EndContextMap(const IMapItemAccessor* handle, Value& context) noexcept;

// The template callable `callable` lends to a host's call that is still running on this thread (LentCallable), or
// null: another callable, or one lent to a call that returned. Defined in internal_value.cpp, where LentCallable is
const InternalValue* LentCallableOf(const UserCallable& callable) noexcept;
// Called in a catch handler: rethrows the current exception as the host sees it, the public ErrorInfo for the
// engine's errors (a located RenderFailure keeps its code) and a host's own exception as it is
[[noreturn]] void RethrowAsPublicError();
} // namespace host_call
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_HOST_CALL_CONTEXT_H
