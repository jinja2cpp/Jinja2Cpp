#ifndef JINJA2CPP_PYTHON_FIBER_H
#define JINJA2CPP_PYTHON_FIBER_H

#include <cstddef>
#include <exception>
#include <functional>

namespace jinja2cpp_fiber
{
// A stack of its own that code runs on, switched to and from on one thread (docs/tasks/0457): the
// Python module runs a lazy Template.generate() on one and suspends it in its sink's Write. No
// Python here, so that the engine's tests run the same switch (test/suspendable_render_test.cpp).
//
// - Linux and macOS on x86-64 and AArch64: boostorg/context's fcontext switch
//   (thirdparty/boost_context), on an mmap'd stack with a guard page below it.
// - Windows: the OS fiber API.
// - Anywhere else, or on Linux with a CET shadow stack enabled, Available() is false.
//
// AddressSanitizer and ThreadSanitizer are told about every switch.
class Fiber
{
public:
    // The default reserve: as much as a thread has, as CPython's recursion counters and the
    // engine's own checks are sized for it. Committed as it is touched
    static constexpr std::size_t DefaultStackSize = std::size_t{ 8 } << 20;

    // Whether fibers work in this process
    [[nodiscard]] static bool Available() noexcept;

    // A fiber that runs `entry` when first resumed. `entry` may call Suspend() and must return
    // normally: an exception it lets out is kept and rethrown by the Resume() that ends it.
    // Throws std::bad_alloc when the stack cannot be reserved, std::logic_error when !Available()
    explicit Fiber(std::function<void()> entry, std::size_t stackSize = DefaultStackSize);
    // A fiber that has started and not finished is freed without running anything more: what its
    // frames own is leaked
    ~Fiber();
    Fiber(const Fiber&) = delete;
    Fiber(Fiber&&) = delete;
    Fiber& operator=(const Fiber&) = delete;
    Fiber& operator=(Fiber&&) = delete;

    // Runs the fiber until it suspends or returns; on the thread that made it, not from the fiber
    // itself. Rethrows what `entry` let out
    void Resume();
    // On the fiber: switches back to the code that resumed it, until the next Resume()
    void Suspend() noexcept;

    [[nodiscard]] bool Started() const noexcept { return m_started; }
    [[nodiscard]] bool Finished() const noexcept { return m_finished; }
    [[nodiscard]] bool Running() const noexcept { return m_running; }
    // The usable stack: its lowest address, above the guard page, and its size. On Windows known
    // only once the fiber has started: read them on the fiber (RenderOptions::suspension), never before
    [[nodiscard]] const void* StackLow() const noexcept { return m_stackLow; }
    [[nodiscard]] std::size_t StackSize() const noexcept { return m_stackSize; }

private:
    struct Impl;
    static void Run(Fiber& fiber) noexcept;
    void SwitchIn();
    void SwitchOut(bool last) noexcept;

    std::function<void()> m_entry;
    std::exception_ptr m_error;
    const void* m_stackLow = nullptr;
    std::size_t m_stackSize = 0;
    bool m_started = false;
    bool m_finished = false;
    bool m_running = false;
    // What one platform or sanitizer uses of the switch (a struct, so that the others leave it unused quietly):
    // Windows, whether Resume made the thread a fiber; ASan, the resumer's stack and the fake stacks of each side;
    // TSan, the fibers of each side
    struct Platform
    {
        // POSIX: the stack's mapping, guard page included
        void* mapping = nullptr;
        std::size_t mappingSize = 0;
        // fcontext: the fiber's and the resumer's saved contexts; Windows: the fiber and the resumer
        void* context = nullptr;
        void* caller = nullptr;
        bool convertedThread = false;
        const void* callerStackBottom = nullptr;
        std::size_t callerStackSize = 0;
        void* callerFakeStack = nullptr;
        void* fiberFakeStack = nullptr;
        void* tsanFiber = nullptr;
        void* tsanCaller = nullptr;
    } m_platform;
};
} // namespace jinja2cpp_fiber

#endif // JINJA2CPP_PYTHON_FIBER_H
