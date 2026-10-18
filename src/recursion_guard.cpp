#include "recursion_guard.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

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
    }
    // A frame below the stack runs on one of its own (a fiber, a coroutine): no check
    return frame >= stack.low && frame < stack.limit;
}

namespace
{
// Steps between two polls of the clock
constexpr std::uint64_t StepsPerPoll = 1024;

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
    if (limits.hasDeadline && std::chrono::steady_clock::now() > limits.deadline)
    {
        ThrowLimit("maxRenderTime", "the render took longer than its time limit");
    }
}

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
        ThrowLimit("maxRenderSteps", "the render took more than " + std::to_string(limits.maxSteps) + " steps");
    }
    PollDeadline();
    Refill(limits);
}
} // namespace detail

RenderLimitsScope::RenderLimitsScope(const Settings::Limits& limits)
    : m_saved(detail::t_renderLimits)
{
    detail::RenderLimits state;
    state.maxSteps = limits.maxRenderSteps;
    if (limits.maxSequenceSize != 0)
    {
        state.maxSequenceSize = std::min(state.maxSequenceSize, limits.maxSequenceSize);
    }
    if (limits.maxRange != 0)
    {
        state.maxRange = limits.maxRange;
    }
    if (limits.maxTemplateLoads != 0)
    {
        state.loadsLeft = limits.maxTemplateLoads;
    }
    state.maxRenderDepth = LimitOrNone(limits.maxRenderDepth);
    if (limits.maxRenderTime.count() > 0)
    {
        state.hasDeadline = true;
        state.deadline = std::chrono::steady_clock::now() + limits.maxRenderTime;
    }
    detail::Refill(state);
    detail::t_renderLimits = state;
}

void CountTemplateLoad()
{
    auto& limits = detail::t_renderLimits;
    if (limits.loadsLeft == 0)
    {
        detail::ThrowLimit("maxTemplateLoads", "the render loaded more templates than its limit allows");
    }
    if (limits.loadsLeft != UINT32_MAX)
    {
        --limits.loadsLeft;
    }
}

void CheckRangeSize(std::uint64_t size)
{
    const auto limit = detail::t_renderLimits.maxRange;
    if (size > limit)
    {
        detail::ThrowLimit("maxRange", "range() of " + std::to_string(size) + " items is longer than " + std::to_string(limit));
    }
}

} // namespace jinja2
