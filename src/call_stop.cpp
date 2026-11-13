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
// The exception KeepFailure kept, between its handler and RethrowKept or RethrowKeptSealed; one at a time per thread
thread_local std::exception_ptr t_kept;

HostCallableFailure SealedFailure(std::exception_ptr exception)
{
    HostCallableFailure failure(std::move(exception));
    failure.SealStop();
    return failure;
}

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

// Whether `exception` is a stop. Outside any handler: the classifier's rethrow then runs on the stack of the call site,
// not on top of the stack of the throw, which MSVC keeps while a handler runs (a failure of the recursion guard
// comes from a deep stack)
bool IsStopException(const std::exception_ptr& exception) noexcept
{
    if (!exception)
    {
        return false;
    }
    try
    {
        std::rethrow_exception(exception);
    }
    catch (const RenderFailure& failure)
    {
        const auto code = failure.Code();
        return !failure.StopSealed() && (code == ErrorCode::UserCallableException || code == ErrorCode::UnexpectedException) && HoldsStop(failure.KeptException());
    }
    catch (const HostCallableFailure& failure)
    {
        return !failure.StopSealed() && HoldsStop(failure.Exception());
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

bool IsStop(const std::exception_ptr& failure) noexcept
{
    return IsStopException(failure);
}

void KeepFailure() noexcept
{
    t_kept = std::current_exception();
}

bool KeptIsStop() noexcept
{
    return IsStopException(t_kept);
}

void RethrowKeptSealed()
{
    auto failure = std::exchange(t_kept, nullptr);
    try
    {
        std::rethrow_exception(std::move(failure));
    }
    catch (HostCallableFailure& sealed)
    {
        sealed.SealStop();
        throw;
    }
    catch (RenderFailure& sealed)
    {
        sealed.SealStop();
        throw;
    }
    catch (const StopIteration&)
    {
        // A host object's own exception: sealed in the failure that holds it
        throw SealedFailure(std::current_exception());
    }
}

std::exception_ptr TakeKept() noexcept
{
    return std::exchange(t_kept, nullptr);
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
