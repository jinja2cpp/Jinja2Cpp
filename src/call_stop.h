#ifndef JINJA2CPP_SRC_CALL_STOP_H
#define JINJA2CPP_SRC_CALL_STOP_H

#include "internal_value.h"

namespace jinja2
{
inline namespace v2
{
class RenderContext;

namespace detail
{
// Jinja2 makes a call of the template (a macro, caller(), loop(...), a callable) that raises StopIteration an
// undefined value (Context.call, docs/tasks/0479). The host marks such an exception with jinja2::StopIteration.
// A call site guards the invocation in the shape of the body loops (renderer.h), which keeps the rethrow outside
// the handler (MSVC runs a handler on top of the stack of the throw):
//
//     bool stopped = false;
//     try { ...; return; }
//     catch (...) { stopped = detail::KeepFailure(); }
//     if (!stopped || !<the output could be rolled back>) detail::RethrowKept();
//     // a stop: undo what the call left (HomeCall::Snapshot), then
//     detail::DropKept();  // and give MakeStopUndefined
//
// A stop is a RenderFailure that is not sealed (SealStop), with code UserCallableException or UnexpectedException
// and a marked exception kept; a HostCallableFailure or a raw exception that holds or is a marked exception. The
// engine's own errors and std::bad_alloc are not.

// Called from a catch handler: keeps the exception being handled for RethrowKept, and tells whether it is a stop
[[nodiscard]] bool KeepFailure() noexcept;
// Called from a catch handler where Jinja2 applies a callable inside a generator (the item filter or test or
// attribute of map, select, reject and their attr forms, the filter of a loop): a StopIteration there is a
// RuntimeError, so what is thrown is no stop for a call site around it. Rethrows the exception being handled,
// sealed; one that is not a stop goes on unchanged
[[noreturn]] void RethrowSealed();
// Called from a catch handler: whether the exception being handled is a stop, without keeping it. For a frame that
// ends as the exception passes it and rethrows the exception it holds, outside the handler
[[nodiscard]] bool IsStopBeingHandled() noexcept;
// Throws the exception KeepFailure kept, outside the handler
[[noreturn]] void RethrowKept();
// Forgets the exception KeepFailure kept
void DropKept() noexcept;
// The undefined value a stopped call gives: the render's policy, Jinja2's message
InternalValue MakeStopUndefined(const RenderContext& context);
} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_CALL_STOP_H
