#include "fiber.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <new>
#include <stdexcept>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(JINJA2CPP_FIBER_FCONTEXT)
#include <sys/mman.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/prctl.h>
#include <sys/syscall.h>
#endif
#endif

#ifdef __has_feature
#if __has_feature(address_sanitizer)
#define JINJA2CPP_FIBER_ASAN 1
#endif
#if __has_feature(thread_sanitizer)
#define JINJA2CPP_FIBER_TSAN 1
#endif
#endif
#ifdef __SANITIZE_ADDRESS__
#define JINJA2CPP_FIBER_ASAN 1
#endif
#ifdef __SANITIZE_THREAD__
#define JINJA2CPP_FIBER_TSAN 1
#endif
#if defined(_WIN32)
// The OS switches the stacks, and MSVC's sanitizers know its fibers
#undef JINJA2CPP_FIBER_ASAN
#undef JINJA2CPP_FIBER_TSAN
#endif
#ifdef JINJA2CPP_FIBER_ASAN
#include <sanitizer/asan_interface.h>
#include <sanitizer/common_interface_defs.h>
#endif
#ifdef JINJA2CPP_FIBER_TSAN
#include <sanitizer/tsan_interface.h>
#endif

#if !defined(_WIN32) && defined(JINJA2CPP_FIBER_FCONTEXT)
// boostorg/context's switch (thirdparty/boost_context), hidden in this library
extern "C"
{
    struct jinja2cpp_fiber_transfer // NOLINT(readability-identifier-naming): fcontext's transfer_t
    {
        void* fctx;
        void* data;
    };
    // NOLINTNEXTLINE(readability-identifier-naming): the assembly's names
    jinja2cpp_fiber_transfer jump_fcontext(void* to, void* data);
    // NOLINTNEXTLINE(readability-identifier-naming)
    void* make_fcontext(void* stackTop, std::size_t size, void (*entry)(jinja2cpp_fiber_transfer));
}
#endif

namespace jinja2cpp_fiber
{
#if defined(_WIN32)
struct Fiber::Impl
{
    static void WINAPI Entry(void* param)
    {
        auto& fiber = *static_cast<Fiber*>(param);
        ULONG_PTR low = 0;
        ULONG_PTR high = 0;
        GetCurrentThreadStackLimits(&low, &high);
        fiber.m_stackLow = reinterpret_cast<const void*>(low);
        fiber.m_stackSize = static_cast<std::size_t>(high - low);
        Run(fiber);
        fiber.SwitchOut(true);
    }
};

bool Fiber::Available() noexcept
{
    return true;
}

Fiber::Fiber(std::function<void()> entry, std::size_t stackSize)
    : m_entry(std::move(entry))
{
    m_platform.context = CreateFiberEx(64 * 1024, stackSize, FIBER_FLAG_FLOAT_SWITCH, &Impl::Entry, this);
    if (!m_platform.context)
    {
        throw std::bad_alloc();
    }
}

Fiber::~Fiber()
{
    DeleteFiber(m_platform.context);
}

void Fiber::SwitchIn()
{
    if (!IsThreadAFiber())
    {
        if (!ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH))
        {
            throw std::bad_alloc();
        }
        m_platform.convertedThread = true;
    }
    m_platform.caller = GetCurrentFiber();
    SwitchToFiber(m_platform.context);
    if (m_platform.convertedThread)
    {
        m_platform.convertedThread = false;
        ConvertFiberToThread();
    }
}

void Fiber::SwitchOut(bool /*last*/) noexcept
{
    SwitchToFiber(m_platform.caller);
}
#elif defined(JINJA2CPP_FIBER_FCONTEXT)
namespace
{
std::size_t PageSize() noexcept
{
    const long size = sysconf(_SC_PAGESIZE);
    return size > 0 ? static_cast<std::size_t>(size) : 4096;
}
} // namespace

struct Fiber::Impl
{
    static void Entry(jinja2cpp_fiber_transfer from) noexcept
    {
        auto& fiber = *static_cast<Fiber*>(from.data);
        fiber.m_platform.caller = from.fctx;
#ifdef JINJA2CPP_FIBER_ASAN
        // The first switch in: no fake stack of this fiber's to restore
        __sanitizer_finish_switch_fiber(nullptr, &fiber.m_platform.callerStackBottom, &fiber.m_platform.callerStackSize);
#endif
        Run(fiber);
        fiber.SwitchOut(true);
    }
};

bool Fiber::Available() noexcept
{
#if defined(__linux__) && defined(__x86_64__)
    // A CET shadow stack would see each switch as a return to the wrong place
    constexpr int archShstkStatus = 0x5005;
    unsigned long features = 0;
    if (syscall(SYS_arch_prctl, archShstkStatus, &features) == 0 && (features & 1U) != 0)
    {
        return false;
    }
#elif defined(__linux__) && defined(__aarch64__)
    // The same for an AArch64 guarded control stack (PR_GET_SHADOW_STACK_STATUS)
    constexpr int getShadowStackStatus = 74;
    unsigned long status = 0;
    if (prctl(getShadowStackStatus, &status, 0, 0, 0) == 0 && (status & 1U) != 0)
    {
        return false;
    }
#endif
    return true;
}

Fiber::Fiber(std::function<void()> entry, std::size_t stackSize)
    : m_entry(std::move(entry))
{
    if (!Available())
    {
        throw std::logic_error("jinja2cpp: fibers are not available in this process");
    }
    const std::size_t page = PageSize();
    const std::size_t size = (stackSize + page - 1) / page * page;
    m_platform.mappingSize = size + page;
#if defined(__linux__)
    constexpr int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_STACK;
#else
    constexpr int flags = MAP_PRIVATE | MAP_ANON;
#endif
    void* mapping = mmap(nullptr, m_platform.mappingSize, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (mapping == MAP_FAILED)
    {
        throw std::bad_alloc();
    }
    // The guard page below the stack: an overflow faults rather than write over other memory
    if (mprotect(mapping, page, PROT_NONE) != 0)
    {
        munmap(mapping, m_platform.mappingSize);
        throw std::bad_alloc();
    }
    m_platform.mapping = mapping;
    auto* low = static_cast<unsigned char*>(mapping) + page;
    m_stackLow = low;
    m_stackSize = size;
    m_platform.context = make_fcontext(low + size, size, &Impl::Entry);
#ifdef JINJA2CPP_FIBER_TSAN
    m_platform.tsanFiber = __tsan_create_fiber(0);
#endif
}

Fiber::~Fiber()
{
#ifdef JINJA2CPP_FIBER_TSAN
    __tsan_destroy_fiber(m_platform.tsanFiber);
#endif
#ifdef JINJA2CPP_FIBER_ASAN
    // Frames of an unfinished fiber leave their red zones poisoned: memory mapped here later must not see them
    __asan_unpoison_memory_region(m_stackLow, m_stackSize);
#endif
    munmap(m_platform.mapping, m_platform.mappingSize);
}

void Fiber::SwitchIn()
{
#ifdef JINJA2CPP_FIBER_ASAN
    __sanitizer_start_switch_fiber(&m_platform.callerFakeStack, m_stackLow, m_stackSize);
#endif
#ifdef JINJA2CPP_FIBER_TSAN
    m_platform.tsanCaller = __tsan_get_current_fiber();
    __tsan_switch_to_fiber(m_platform.tsanFiber, 0);
#endif
    const auto back = jump_fcontext(m_platform.context, this);
    m_platform.context = back.fctx;
#ifdef JINJA2CPP_FIBER_ASAN
    __sanitizer_finish_switch_fiber(m_platform.callerFakeStack, nullptr, nullptr);
#endif
}

void Fiber::SwitchOut(bool last) noexcept
{
#ifdef JINJA2CPP_FIBER_ASAN
    // The last switch out leaves no fake stack behind: the fiber never runs again
    __sanitizer_start_switch_fiber(last ? nullptr : &m_platform.fiberFakeStack, m_platform.callerStackBottom, m_platform.callerStackSize);
#else
    static_cast<void>(last);
#endif
#ifdef JINJA2CPP_FIBER_TSAN
    __tsan_switch_to_fiber(m_platform.tsanCaller, 0);
#endif
    const auto back = jump_fcontext(m_platform.caller, this);
    // Resumed, perhaps from another stack than the last time
    m_platform.caller = back.fctx;
#ifdef JINJA2CPP_FIBER_ASAN
    __sanitizer_finish_switch_fiber(m_platform.fiberFakeStack, &m_platform.callerStackBottom, &m_platform.callerStackSize);
#endif
}
#else
struct Fiber::Impl
{
};

bool Fiber::Available() noexcept
{
    return false;
}

Fiber::Fiber(std::function<void()> entry, std::size_t /*stackSize*/)
    : m_entry(std::move(entry))
{
    throw std::logic_error("jinja2cpp: fibers are not available on this platform");
}

Fiber::~Fiber() = default;

void Fiber::SwitchIn()
{
}

void Fiber::SwitchOut(bool /*last*/) noexcept
{
}
#endif

void Fiber::Run(Fiber& fiber) noexcept
{
    try
    {
        fiber.m_entry();
    }
    catch (...)
    {
        fiber.m_error = std::current_exception();
    }
    fiber.m_finished = true;
}

void Fiber::Resume()
{
    if (m_running || m_finished)
    {
        throw std::logic_error("jinja2cpp: a fiber was resumed while it runs or after it finished");
    }
    m_running = true;
    m_started = true;
    try
    {
        SwitchIn();
    }
    catch (...)
    {
        // Windows could not make the thread a fiber: nothing ran
        m_running = false;
        throw;
    }
    m_running = false;
    if (m_finished && m_error)
    {
        std::rethrow_exception(std::exchange(m_error, nullptr));
    }
}

void Fiber::Suspend() noexcept
{
    SwitchOut(false);
}
} // namespace jinja2cpp_fiber
