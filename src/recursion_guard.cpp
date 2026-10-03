#include "recursion_guard.h"

#include <cstddef>
#include <cstdint>

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
} // namespace detail

} // namespace jinja2
