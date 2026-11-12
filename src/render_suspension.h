#ifndef JINJA2CPP_SRC_RENDER_SUSPENSION_H
#define JINJA2CPP_SRC_RENDER_SUSPENSION_H

#include "recursion_guard.h"

#include <jinja2cpp/template.h>

#include <cstddef>
#include <memory>
#include <string_view>

namespace jinja2
{
inline namespace v2
{
class CycleCollector;
class RenderWorkspace;

namespace detail
{
// What the code of values keeps per thread between two steps of a render, outside the bundle
// below: the same before and after each Write of a suspendable render, or the switch would hand it
// to another render (debug check, docs/tasks/0457)
struct SuspensionMarks
{
    unsigned destroyDepth = 0;
    std::size_t pendingDestroy = 0;
    const void* bridgingAccessor = nullptr;
    const void* lendFrame = nullptr;
    bool keepUndefinedExport = false;
    bool renderFailure = false;

    [[nodiscard]] bool operator==(const SuspensionMarks& other) const noexcept
    {
        return destroyDepth == other.destroyDepth && pendingDestroy == other.pendingDestroy && bridgingAccessor == other.bridgingAccessor
               && lendFrame == other.lendFrame && keepUndefinedExport == other.keepUndefinedExport && renderFailure == other.renderFailure;
    }
};
// internal_value.cpp and render_failure.cpp
SuspensionMarks ValueSuspensionMarks() noexcept;
bool HasPendingRenderFailure() noexcept;

// The per-render state of the thread a suspendable render runs on (RenderOptions::suspension,
// docs/tasks/0457). Renders on a thread nest: each saves what it changes of the thread's state and
// puts it back when it ends. A render that lets the thread go in the middle (its sink's Write
// switches to the host, which may start, run and finish other renders before it switches back)
// would break that order, so it keeps its own copy of that state, and Exchange swaps it with the
// thread's: when the render starts, before and after each Write, and when it ends. Every
// per-render thread-local of the engine is in here, or is shown safe to share
// (docs/design/0457-lazy-generate-plan.md, section 3.1):
// - in here: the workspace's contents and its count of running renders, the render limits, the
//   stack bounds, the host's poll, the render depth, the cycle collector, the parse limits;
// - idle at every Write (SuspensionMarks): deferred destructions, a pending render failure, a
//   bridging accessor, a lent frame, the export mode of undefined values;
// - shared: the lookup cache (its epochs never repeat), the HTML escape buffer (a sink's stream
//   keeps no fragment in it), the JSON node type cache, the slot hash seed, the cell trace.
struct RenderThreadState
{
    // Starts with nothing running: an empty workspace, no limits, poll, collector or depth, and
    // `suspension`'s stack. Throws std::bad_alloc
    explicit RenderThreadState(const RenderSuspension& suspension);
    ~RenderThreadState();
    RenderThreadState(const RenderThreadState&) = delete;
    RenderThreadState(RenderThreadState&&) = delete;
    RenderThreadState& operator=(const RenderThreadState&) = delete;
    RenderThreadState& operator=(RenderThreadState&&) = delete;

    // Swaps this state with the thread's
    void Exchange() noexcept;
    // Debug: what the values' thread-locals hold now (RecordMarks), when the render gets the thread
    void RecordMarks() noexcept;
    // Debug: they hold the same when it lets the thread go, so the host finds them as it left them
    void CheckIdle() const noexcept;

    std::unique_ptr<RenderWorkspace> workspace;
    std::size_t renders = 0;
    RenderLimits limits;
    ThreadStack stack;
    HostPoll poll = nullptr;
    unsigned depth = 0;
    CycleCollector* collector = nullptr;
    ParseLimits parseLimits;
    SuspensionMarks marks;
};

// Frees the workspaces kept for the next suspendable renders on this thread (ReleaseThreadCaches)
bool ReleaseSuspensionPool() noexcept;

// A render's state swapped in for its scope (RenderCore, before anything it does)
class SuspendedRenderScope
{
public:
    explicit SuspendedRenderScope(const RenderSuspension& suspension)
        : m_state(suspension)
    {
        m_state.Exchange();
        m_state.RecordMarks();
    }
    ~SuspendedRenderScope() { m_state.Exchange(); }
    SuspendedRenderScope(const SuspendedRenderScope&) = delete;
    SuspendedRenderScope(SuspendedRenderScope&&) = delete;
    SuspendedRenderScope& operator=(const SuspendedRenderScope&) = delete;
    SuspendedRenderScope& operator=(SuspendedRenderScope&&) = delete;

    [[nodiscard]] RenderThreadState& State() noexcept { return m_state; }

private:
    RenderThreadState m_state;
};

// The host's sink of a suspendable render: the thread's state is the host's while its Write runs,
// and the render's again when it returns or throws
template<typename CharT>
class DetachingSink final : public BasicOutputSink<CharT>
{
public:
    DetachingSink(BasicOutputSink<CharT>& sink, RenderThreadState& state) noexcept
        : m_sink(sink)
        , m_state(state)
    {
    }
    DetachingSink(const DetachingSink&) = delete;
    DetachingSink(DetachingSink&&) = delete;
    DetachingSink& operator=(const DetachingSink&) = delete;
    DetachingSink& operator=(DetachingSink&&) = delete;
    ~DetachingSink() = default;

    void Write(std::basic_string_view<CharT> text) override;

private:
    BasicOutputSink<CharT>& m_sink;
    RenderThreadState& m_state;
};
} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_RENDER_SUSPENSION_H
