#ifndef JINJA2CPP_SRC_CALL_STOP_H
#define JINJA2CPP_SRC_CALL_STOP_H

#include "internal_value.h"

#include <exception>

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
//     bool failed = false;
//     try { ...; return; }
//     catch (...) { failed = true; detail::KeepFailure(); }
//     if (!detail::KeptIsStop() || !<the output could be rolled back>) detail::RethrowKept();
//     // a stop: undo what the call left (HomeCall::Snapshot), then
//     detail::DropKept();  // and give MakeStopUndefined
//
// Only KeepFailure runs in the handler: the classifier rethrows the exception, which MSVC would do on top of the
// stack of the throw while a handler runs.
//
// A stop is a RenderFailure that is not sealed (SealStop), with code UserCallableException or UnexpectedException
// and a marked exception kept; a HostCallableFailure or a raw exception that holds or is a marked exception. The
// engine's own errors and std::bad_alloc are not.

// Called from a catch handler: keeps the exception being handled for KeptIsStop, RethrowKept or RethrowKeptSealed
void KeepFailure() noexcept;
// Outside the handler: whether the exception KeepFailure kept is a stop
[[nodiscard]] bool KeptIsStop() noexcept;
// Whether `failure` is a stop; call it outside the handler that took it
[[nodiscard]] bool IsStop(const std::exception_ptr& failure) noexcept;
// Outside the handler, where Jinja2 applies a callable inside a generator (the item filter or test or attribute of
// map, select, reject and their attr forms, unique's attribute, the filter of a loop): a StopIteration there is a
// RuntimeError, so the exception KeepFailure kept is thrown sealed, no stop for a call site around it; one that is
// not a stop goes on unchanged
[[noreturn]] void RethrowKeptSealed();
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
