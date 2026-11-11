#include "render_suspension.h"

#include "cycle_collector.h"
#include "recursion_guard.h"
#include "render_workspace.h"

#include <jinja2cpp/template.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
namespace
{
// The workspaces of the suspendable renders that ended on this thread, with the slot chunks and
// tables they grew, for the next ones: a generator per request reuses them as a plain render
// reuses the thread's. A few only, freed by ReleaseThreadCaches
constexpr std::size_t MaxPooledWorkspaces = 4;

std::vector<std::unique_ptr<RenderWorkspace>>& Pool()
{
    thread_local std::vector<std::unique_ptr<RenderWorkspace>> pool;
    return pool;
}

std::unique_ptr<RenderWorkspace> TakeWorkspace()
{
    auto& pool = Pool();
    if (pool.empty())
    {
        return std::make_unique<RenderWorkspace>();
    }
    auto workspace = std::move(pool.back());
    pool.pop_back();
    return workspace;
}
} // namespace

RenderThreadState::RenderThreadState(const RenderSuspension& suspension)
    : workspace(TakeWorkspace())
    , stack(t_threadStack)
{
    // The thread's workspace is made here, where a failed allocation can throw, not in Exchange
    static_cast<void>(RenderWorkspace::ForThisThread());
    if (suspension.stackSize != 0)
    {
        const auto low = reinterpret_cast<std::uintptr_t>(suspension.stackLow);
        stack = StackOf(low, low + suspension.stackSize);
    }
    else if (stack.limit != UINTPTR_MAX)
    {
        // The thread's own stack, without the render limits of a render running there
        stack.renderLimit = stack.limit;
    }
}

RenderThreadState::~RenderThreadState()
{
    // Back with the thread's state: this holds what the render left, which it does not need again
    assert(renders == 0 && depth == 0 && !collector);
    if (workspace->m_lentMaps != 0 || !workspace->m_frames.empty())
    {
        // A render that ended holds nothing of its workspace: keep a broken one out of the pool
        return;
    }
    workspace->ReleaseRetired();
    try
    {
        auto& pool = Pool();
        if (pool.size() < MaxPooledWorkspaces)
        {
            pool.push_back(std::move(workspace));
        }
    }
    catch (...)
    {
        // The pool could not grow: the workspace is freed instead
        static_cast<void>(0);
    }
}

void RenderThreadState::Exchange() noexcept
{
    using std::swap;
    RenderWorkspace::ForThisThread().SwapContents(*workspace);
    swap(RenderWorkspace::RunningRenders(), renders);
    swap(t_renderLimits, limits);
    swap(t_threadStack, stack);
    swap(t_hostPoll, poll);
    swap(RenderDepthGuard::Depth(), depth);
    swap(CycleCollector::s_current, collector);
    swap(t_parseLimits, parseLimits);
}

void RenderThreadState::RecordMarks() noexcept
{
#ifndef NDEBUG
    marks = ValueSuspensionMarks();
    marks.renderFailure = HasPendingRenderFailure();
#endif
}

void RenderThreadState::CheckIdle() const noexcept
{
#ifndef NDEBUG
    auto now = ValueSuspensionMarks();
    now.renderFailure = HasPendingRenderFailure();
    assert(now == marks);
#endif
}

bool ReleaseSuspensionPool() noexcept
{
    // Moved out first: what a workspace holds may run host code when it goes
    auto pool = std::move(Pool());
    Pool().clear();
    for (auto& workspace : pool)
    {
        workspace->ReleaseCaches();
    }
    return true;
}

template<typename CharT>
void DetachingSink<CharT>::Write(std::basic_string_view<CharT> text)
{
    m_state.CheckIdle();
    // The render's state goes to m_state for the host's Write, and comes back whatever it does
    struct Swapped
    {
        RenderThreadState& state;
        explicit Swapped(RenderThreadState& s) noexcept
            : state(s)
        {
            state.Exchange();
        }
        ~Swapped()
        {
            state.Exchange();
            state.RecordMarks();
        }
        Swapped(const Swapped&) = delete;
        Swapped(Swapped&&) = delete;
        Swapped& operator=(const Swapped&) = delete;
        Swapped& operator=(Swapped&&) = delete;
    } const swapped(m_state);
    m_sink.Write(text);
}

template class DetachingSink<char>;
template class DetachingSink<wchar_t>;
} // namespace detail
} // namespace v2
} // namespace jinja2
