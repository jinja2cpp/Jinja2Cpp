#include "call_stop.h"

#include "engine_errors.h"
#include "internal_value.h"
#include "render_failure.h"
#include "undefined.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/exception.h>

#include <exception>
#include <utility>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
namespace
{
// The exception KeepUnlessStop kept, between its handler and RethrowKept; one at a time per thread
thread_local std::exception_ptr t_kept;

bool HoldsStop(const std::exception_ptr& exception)
{
    if (!exception)
    {
        return false;
    }
    try
    {
        std::rethrow_exception(exception);
    }
    catch (const StopIteration&)
    {
        return true;
    }
    catch (...) // NOLINT(bugprone-empty-catch): any other exception is no stop
    {
    }
    return false;
}

// Whether the exception being handled is a stop
bool IsStop() noexcept
{
    try
    {
        throw;
    }
    catch (const RenderFailure& failure)
    {
        const auto code = failure.Code();
        return !failure.StopSealed() && (code == ErrorCode::UserCallableException || code == ErrorCode::UnexpectedException) && HoldsStop(failure.KeptException());
    }
    catch (const HostCallableFailure& failure)
    {
        return HoldsStop(failure.Exception());
    }
    catch (const StopIteration&)
    {
        return true;
    }
    catch (...) // NOLINT(bugprone-empty-catch): any other exception is no stop
    {
    }
    return false;
}
} // namespace

bool KeepFailure() noexcept
{
    t_kept = std::current_exception();
    return IsStop();
}

void RethrowKept()
{
    std::rethrow_exception(std::exchange(t_kept, nullptr));
}

void DropKept() noexcept
{
    t_kept = nullptr;
}

InternalValue MakeStopUndefined(const RenderContext& context)
{
    return MakeUndefinedWithHint(context, "value was undefined because a callable raised a StopIteration exception");
}
} // namespace detail
} // namespace v2
} // namespace jinja2
