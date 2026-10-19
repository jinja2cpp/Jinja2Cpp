#include "recursion_guard.h"

#include <jinja2cpp/template_env.h>

#include <algorithm>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__)
#include <pthread.h>
#ifdef __FreeBSD__
#include <pthread_np.h>
#endif
#endif

#ifdef __has_feature
#if __has_feature(address_sanitizer)
#define JINJA2CPP_ASAN 1
#endif
#endif
#ifdef __SANITIZE_ADDRESS__
#define JINJA2CPP_ASAN 1
#endif

namespace jinja2
{
inline namespace v2
{
namespace
{
// Stack left free for the work between two checks: a filter, a value conversion, the
// frames of one expression or statement. ASan pads every frame with red zones.
#ifdef JINJA2CPP_ASAN
constexpr std::uintptr_t StackReserve = std::uintptr_t{ 512 } * 1024;
#else
constexpr std::uintptr_t StackReserve = std::uintptr_t{ 128 } * 1024;
#endif

struct StackBounds
{
    // [low, high) is the current thread's stack; zero when unknown
    std::uintptr_t low = 0;
    std::uintptr_t high = 0;
};

StackBounds CurrentThreadStack()
{
    StackBounds bounds;
#ifdef _WIN32
    ULONG_PTR low = 0;
    ULONG_PTR high = 0;
    GetCurrentThreadStackLimits(&low, &high);
    bounds.low = static_cast<std::uintptr_t>(low);
    bounds.high = static_cast<std::uintptr_t>(high);
#elif defined(__APPLE__)
    pthread_t self = pthread_self();
    bounds.high = reinterpret_cast<std::uintptr_t>(pthread_get_stackaddr_np(self));
    bounds.low = bounds.high - pthread_get_stacksize_np(self);
#elif defined(__linux__) || defined(__FreeBSD__)
    pthread_attr_t attr;
#ifdef __FreeBSD__
    if (pthread_attr_init(&attr) != 0)
    {
        return bounds;
    }
    if (pthread_attr_get_np(pthread_self(), &attr) == 0)
#else
    if (pthread_getattr_np(pthread_self(), &attr) == 0)
#endif
    {
        void* addr = nullptr;
        std::size_t size = 0;
        if (pthread_attr_getstack(&attr, &addr, &size) == 0)
        {
            bounds.low = reinterpret_cast<std::uintptr_t>(addr);
            bounds.high = bounds.low + size;
        }
        pthread_attr_destroy(&attr);
    }
#endif
    return bounds;
}

} // namespace

namespace detail
{
bool StackNearlyExhaustedSlow(std::uintptr_t frame)
{
    auto& stack = t_threadStack;
    if (stack.limit == UINTPTR_MAX)
    {
        const auto bounds = CurrentThreadStack();
        // A stack too small for the reserve, or of unknown bounds, is not checked
        const bool known = bounds.high - bounds.low > StackReserve;
        stack.low = known ? bounds.low : 0;
        stack.limit = known ? bounds.low + StackReserve : 0;
        if (!t_renderLimits.active)
        {
            stack.renderLimit = stack.limit;
        }
    }
    // A frame below the stack runs on one of its own (a fiber, a coroutine): no check
    return frame >= stack.low && frame < stack.limit;
}

namespace
{
// Steps between two polls of the clock: few, as one step may be a long filter
constexpr std::uint64_t StepsPerPoll = 16;

// Starts the next countdown: to the end of the step budget, or to the next clock poll
void Refill(RenderLimits& limits)
{
    std::uint64_t chunk = limits.hasDeadline ? StepsPerPoll : UINT64_MAX;
    if (limits.maxSteps != 0)
    {
        // The step after the last allowed one reaches the slow path, which fails it
        chunk = std::min(chunk, limits.maxSteps + 1 - limits.stepsTaken);
    }
    limits.chunk = chunk;
    limits.stepsLeft = chunk;
}

[[noreturn]] void ThrowLimit(const char* field, std::string message)
{
    throw ResourceLimitError(field, std::move(message));
}
} // namespace

void PollDeadline()
{
    const auto& limits = t_renderLimits;
    if (limits.hasDeadline && std::chrono::steady_clock::now().time_since_epoch().count() > limits.deadline)
    {
        ThrowLimit("maxRenderTime", "the render took longer than its time limit");
    }
}

namespace
{
void StepsSlow()
{
    auto& limits = t_renderLimits;
    limits.stepsTaken += limits.chunk;
    if (limits.maxSteps != 0 && limits.stepsTaken > limits.maxSteps)
    {
        // Every later step fails too
        limits.chunk = 0;
        limits.stepsLeft = 1;
        limits.stepsTaken = limits.maxSteps + 1;
        ThrowLimit("maxRenderSteps", "the render took more than " + std::to_string(limits.maxSteps) + " step(s)");
    }
    PollDeadline();
    Refill(limits);
}
} // namespace

void CheckStackSlow(std::uintptr_t frame)
{
    if (t_renderLimits.active)
    {
        if (--t_renderLimits.stepsLeft == 0)
        {
            StepsSlow();
        }
        if (t_renderLimits.output)
        {
            CheckRenderOutput();
        }
    }
    if (StackNearlyExhaustedSlow(frame))
    {
        throw RecursionLimitError();
    }
}

void InstallRenderLimits(const RenderLimits& limits)
{
    t_renderLimits = limits;
    auto& stack = t_threadStack;
    stack.renderLimit = limits.active ? UINTPTR_MAX : stack.limit;
}
} // namespace detail

void RenderLimitsScope::Install(const Settings::Limits* limitsOrNone)
{
    m_saved = detail::t_renderLimits;
    detail::RenderLimits state;
    if (!limitsOrNone)
    {
        detail::InstallRenderLimits(state);
        return;
    }
    const auto& limits = *limitsOrNone;
    state.active = true;
    // UINT64_MAX steps cannot be counted to their end: it is no limit
    state.maxSteps = limits.maxRenderSteps != UINT64_MAX ? limits.maxRenderSteps : 0;
    if (limits.maxSequenceSize != 0)
    {
        state.maxSequenceSize = std::min(state.maxSequenceSize, limits.maxSequenceSize);
    }
    if (limits.maxRange != 0)
    {
        state.maxRange = limits.maxRange;
    }
    if (limits.maxOutputSize != 0)
    {
        // Made an end by LimitRenderOutput
        state.maxOutputEnd = limits.maxOutputSize < SIZE_MAX ? static_cast<std::size_t>(limits.maxOutputSize) : SIZE_MAX - 1;
    }
    if (limits.maxTemplateLoads != 0)
    {
        state.loadsLeft = limits.maxTemplateLoads;
    }
    state.maxRenderDepth = LimitOrNone(limits.maxRenderDepth);
    if (limits.maxRenderTime.count() > 0)
    {
        // A time past the clock's range (milliseconds::max() for "forever") is no limit
        const auto now = std::chrono::steady_clock::now();
        const auto room = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::time_point::max() - now);
        if (limits.maxRenderTime < room)
        {
            state.hasDeadline = true;
            state.deadline = (now + limits.maxRenderTime).time_since_epoch().count();
        }
    }
    detail::Refill(state);
    detail::InstallRenderLimits(state);
}

void CheckRenderOutput()
{
    const auto& limits = detail::t_renderLimits;
    if (!limits.output)
    {
        return;
    }
    const auto size = limits.wideOutput ? static_cast<const std::wstring*>(limits.output)->size() : static_cast<const std::string*>(limits.output)->size();
    if (size > limits.maxOutputEnd)
    {
        detail::ThrowLimit("maxOutputSize", "the render wrote more characters than its output limit allows");
    }
}

namespace detail
{
void CountTemplateLoadSlow()
{
    auto& limits = t_renderLimits;
    if (limits.loadsLeft == 0)
    {
        ThrowLimit("maxTemplateLoads", "the render loaded more templates than its limit allows");
    }
    --limits.loadsLeft;
}

void ThrowRangeTooLong(std::uint64_t size)
{
    ThrowLimit("maxRange", "range() of " + std::to_string(size) + " items is longer than " + std::to_string(t_renderLimits.maxRange));
}
} // namespace detail

} // namespace v2
} // namespace jinja2
