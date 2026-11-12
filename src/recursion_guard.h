#ifndef JINJA2CPP_SRC_RECURSION_GUARD_H
#define JINJA2CPP_SRC_RECURSION_GUARD_H

#include "dialect.h"
#include "engine_errors.h"

#include <jinja2cpp/big_int_backend.h>
#include <jinja2cpp/template_env.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

namespace jinja2
{
inline namespace v2
{

// Limits that keep hostile or runaway templates from overflowing the stack (docs/tasks/0003).
//
// Two layers. The counts below stop each kind of nesting near where Python Jinja2 stops
// the same template with RecursionError or SyntaxError, so errors match Python's. They
// cannot bound stack use, which is their product (250 macro calls, each evaluating a long
// expression) and depends on the thread's stack, so the parser, the evaluator and the
// renderer also check how much of the current thread's stack is left
// (StackNearlyExhausted) and stop there with the same error.
//
// The counts are Settings::Limits: maxExpressionDepth, maxExpressionOperators and
// maxBlockNesting, which the parser reads from ParseLimits, and maxRenderDepth, which
// RenderDepthGuard reads from the render's RenderLimits.

// A depth limit of Settings::Limits as the counters compare it: 0, no limit, never passed
constexpr unsigned LimitOrNone(std::uint32_t limit)
{
    return limit != 0 ? limit : UINT_MAX;
}

// The thread-locals every render step reads. In the Python module, a library the interpreter
// loads with dlopen, they would take the dynamic TLS model: a call into the dynamic loader at
// each access, a fifth of a render's instructions. The module's build (JINJA2CPP_BUILD_PYTHON)
// gives these few the initial-exec model, which the loader's static TLS reserve has room for
// An error path or a rare mode kept out of the function it is called from
#ifdef _MSC_VER
#define JINJA2CPP_NOINLINE __declspec(noinline)
#else
#define JINJA2CPP_NOINLINE __attribute__((noinline))
#endif

#ifdef JINJA2CPP_HOT_TLS_INITIAL_EXEC
#define JINJA2CPP_HOT_TLS __attribute__((tls_model("initial-exec")))
#else
#define JINJA2CPP_HOT_TLS
#endif

namespace detail
{
// The current thread's stack below `limit` is the reserve that StackNearlyExhausted keeps
// free; `low` is its end. The bounds are read on the first check in a thread, which always
// takes the slow path; a stack of unknown bounds gets limit 0 and is never checked again.
// `renderLimit` is what CheckStack, the renderer's check, compares with: `limit`, or
// UINTPTR_MAX while a render with limits runs, so that every check of that render takes the
// slow path, which counts it as a step (Settings::Limits::maxRenderSteps). A render without
// limits pays nothing for them.
struct ThreadStack
{
    std::uintptr_t limit = UINTPTR_MAX;
    std::uintptr_t low = 0;
    std::uintptr_t renderLimit = UINTPTR_MAX;
};
inline thread_local ThreadStack t_threadStack JINJA2CPP_HOT_TLS;

bool StackNearlyExhaustedSlow(std::uintptr_t frame);
// The checks of a stack [low, high) that a render runs on: a thread's, or a fiber's its host switched
// it to (RenderOptions::suspension, docs/tasks/0457). `renderLimit` is `limit`
ThreadStack StackOf(std::uintptr_t low, std::uintptr_t high) noexcept;

struct RenderThreadState;

// The parser's limits and dialect for the template being loaded (ParseLimitsScope), in a
// thread-local rather than in each parser, which lives for one tag
struct ParseLimits
{
    unsigned maxExpressionDepth = 70;
    unsigned maxExpressionOperators = 491;
    unsigned maxBlockNesting = 248;
    Dialect dialect = Dialect::Jinja2;
    // The template is loaded for an environment with a sandbox policy (src/sandbox_policy.h): the nodes that read
    // an attribute of loop, a namespace or a macro are made to ask it, so a render without one pays nothing
    bool sandboxed = false;
};
static_assert(ParseLimits{}.maxExpressionDepth == Settings::Limits{}.maxExpressionDepth
              && ParseLimits{}.maxExpressionOperators == Settings::Limits{}.maxExpressionOperators
              && ParseLimits{}.maxBlockNesting == Settings::Limits{}.maxBlockNesting);
inline thread_local ParseLimits t_parseLimits;

// CheckStack's slow path: counts the step of a render with limits, then checks the stack
void CheckStackSlow(std::uintptr_t frame);

// What the running render may still do (Settings::Limits, docs/tasks/0262). A limit that is
// off holds the largest value of its type, so each check is one compare. Outside a render,
// and in a render without limits, every limit is off.
struct RenderLimits
{
    // Steps left before StepsSlow: the rest of the step budget, or of the 1024 steps between
    // two clock polls, whichever ends first
    std::uint64_t stepsLeft = UINT64_MAX;
    // Steps the countdown started from, and the steps taken before it
    std::uint64_t chunk = UINT64_MAX;
    std::uint64_t stepsTaken = 0;
    std::uint64_t maxSteps = 0;
    // The built-in cap (MaxSequenceSize in internal_value.h) or a lower maxSequenceSize
    std::uint64_t maxSequenceSize = std::uint64_t{ 1 } << 26;
    std::uint64_t maxRange = UINT64_MAX;
    std::uint64_t loadsLeft = UINT64_MAX;
    // Bytes the render's strings, lists and dicts may still take (maxRenderMemory, docs/tasks/0459), and the budget
    // they come from; UINT64_MAX in both without one
    std::uint64_t memoryLeft = UINT64_MAX;
    std::uint64_t memoryBudget = UINT64_MAX;
    // Bytes an object took past the budget where nothing may throw (ChargeObjectMemory): the next step fails the render
    std::uint64_t memoryOverdraft = 0;
    unsigned maxRenderDepth = 256;
    // The render's output string (std::string, or std::wstring when `wideOutput`), or the count
    // of the characters its sink received (`sinkOutput`), and the size it may reach
    // (maxOutputSize); null without that limit
    const void* output = nullptr;
    // The big-integer arithmetic of the render (TemplateEnv::SetBigIntBackend, docs/tasks/0271);
    // null for none. It rides with the limits so that a render without either costs nothing more
    const BigIntBackend* bigInts = nullptr;
    std::size_t maxOutputEnd = SIZE_MAX;
    bool wideOutput = false;
    bool sinkOutput = false;
    // Whether a limit is set: a render without limits inside one with them installs these defaults
    bool active = false;
    // Whether a render installed these for its limits (outside a -DJINJA2CPP_WITH_BIGINT=boost
    // build, also for its big-integer backend): a render without either inside one with them
    // installs the defaults
    bool scoped = false;
    bool hasDeadline = false;
    // A render that writes each piece (RenderOptions::writeEachPiece, EachPieceScope) runs: every check
    // takes the slow path, where a body reads its stream's flag, so the usual render pays nothing for it
    bool eachPiece = false;
    // steady_clock ticks since its epoch: a time_point's constructor is not noexcept, which a
    // thread-local's initialiser has to be
    std::chrono::steady_clock::rep deadline = 0;
};
static_assert(RenderLimits{}.maxRenderDepth == Settings::Limits{}.maxRenderDepth);
// Constant-initialised, so that reading it needs no guard
inline thread_local RenderLimits t_renderLimits JINJA2CPP_HOT_TLS;

void PollDeadline();
void InstallRenderLimits(const RenderLimits& limits);

// A host's poll of the renders on this thread (RenderOptions::poll, docs/tasks/0357 B3; the
// Python module checks for signals there, so Ctrl-C stops a long render, 0353). A render with
// limits calls it every HostPollSteps steps; it may throw to stop the render. Null, the
// default, costs nothing: the slow path of the step count reads it, not the step
using HostPoll = const std::function<void()>*;
inline thread_local HostPoll t_hostPoll JINJA2CPP_HOT_TLS = nullptr;
inline constexpr std::uint64_t HostPollSteps = 1024;
} // namespace detail

// Installs a host's poll for as long as it lives and puts back the one it replaces. A render
// it interrupts, or one it starts with no poll of its own, polls with the same
class HostPollScope
{
public:
    explicit HostPollScope(detail::HostPoll poll) noexcept
        : m_saved(std::exchange(detail::t_hostPoll, poll))
    {
    }
    ~HostPollScope() { detail::t_hostPoll = m_saved; }
    HostPollScope(const HostPollScope&) = delete;
    HostPollScope& operator=(const HostPollScope&) = delete;
    HostPollScope(HostPollScope&&) = delete;
    HostPollScope& operator=(HostPollScope&&) = delete;

private:
    detail::HostPoll m_saved;
};

// The limits of a render that has a poll but no limits of its own: it counts its steps, to
// reach the poll, and limits nothing (UINT64_MAX steps are no limit)
inline const Settings::Limits& PollOnlyLimits()
{
    static const Settings::Limits limits = [] {
        Settings::Limits result;
        result.maxRenderSteps = UINT64_MAX;
        return result;
    }();
    return limits;
}

// Installs a render's limits for as long as it lives and puts back the ones of the render it
// interrupts (a host callable may render another template). A render without limits (null)
// on a thread that runs none costs a test of a thread-local flag
class RenderLimitsScope
{
public:
    // `limits` is null when the render has neither limits nor, outside a -DJINJA2CPP_WITH_BIGINT=boost build,
    // a big-integer backend; `bigInts` is its backend, or null. In a boost build every render has one, and
    // installing the default limits for it cost a short render 17%, so there a backend alone swaps one
    // pointer; the default build keeps its fast path free of it (docs/tasks/0272)
    RenderLimitsScope(const Settings::Limits* limits, const BigIntBackend* bigInts)
    {
        auto& state = detail::t_renderLimits;
        if (limits || state.scoped)
        {
            Install(limits, bigInts);
            return;
        }
#ifdef JINJA2CPP_WITH_BIGINT_BOOST
        // Unconditional: cheaper than testing whether either pointer is set
        m_savedBigInts = state.bigInts;
        state.bigInts = bigInts;
#endif
    }
    ~RenderLimitsScope()
    {
        if (m_saved)
        {
            detail::InstallRenderLimits(*m_saved);
            return;
        }
#ifdef JINJA2CPP_WITH_BIGINT_BOOST
        detail::t_renderLimits.bigInts = m_savedBigInts;
#endif
    }
    RenderLimitsScope(const RenderLimitsScope&) = delete;
    RenderLimitsScope& operator=(const RenderLimitsScope&) = delete;
    RenderLimitsScope(RenderLimitsScope&&) = delete;
    RenderLimitsScope& operator=(RenderLimitsScope&&) = delete;

private:
    void Install(const Settings::Limits* limits, const BigIntBackend* bigInts);

    std::optional<detail::RenderLimits> m_saved;
#ifdef JINJA2CPP_WITH_BIGINT_BOOST
    // The backend of the render this one interrupts, when this one installs no limits
    const BigIntBackend* m_savedBigInts = nullptr;
#endif
};

// Whether a template loaded with these limits renders with any (RenderLimitsScope): the
// Load-time limits aside, one of them differs from the defaults
inline bool HasRenderLimits(const Settings::Limits& limits)
{
    const Settings::Limits defaults;
    return limits.maxRange != 0 || limits.maxSequenceSize != 0 || limits.maxOutputSize != 0 || limits.maxTemplateLoads != 0
           || limits.maxRenderSteps != 0 || limits.maxRenderMemory != 0 || limits.maxRenderTime.count() > 0 || limits.maxRenderDepth != defaults.maxRenderDepth;
}

// Bounds the render's output (Settings::Limits::maxOutputSize): `output` may grow to `start`
// plus that many characters. Checked at every render step and when the render ends, so it
// may pass the limit by the output of one expression and what the stream buffers
template<typename CharT>
void LimitRenderOutput(const std::basic_string<CharT>& output, std::size_t start)
{
    auto& limits = detail::t_renderLimits;
    if (limits.maxOutputEnd != SIZE_MAX)
    {
        limits.output = &output;
        limits.wideOutput = !std::is_same_v<CharT, char>;
        limits.sinkOutput = false;
        limits.maxOutputEnd = limits.maxOutputEnd < SIZE_MAX - start ? start + limits.maxOutputEnd : SIZE_MAX - 1;
    }
}
// The same for a render to a sink: `written` counts the characters the sink received
inline void LimitRenderOutput(const std::size_t& written)
{
    auto& limits = detail::t_renderLimits;
    if (limits.maxOutputEnd != SIZE_MAX)
    {
        limits.output = &written;
        limits.sinkOutput = true;
    }
}
// Throws ResourceLimitError when the render's output is past its limit
void CheckRenderOutput();

// After a host callable returns: a render past its deadline stops here, not 1024 steps later
inline void AfterHostCall()
{
    if (detail::t_renderLimits.hasDeadline)
    {
        detail::PollDeadline();
    }
}

// Installs the parser's limits for the Load it lives for and puts back those of the Load it
// interrupts (a render loads the templates it includes)
class ParseLimitsScope
{
public:
    explicit ParseLimitsScope(const Settings::Limits& limits, detail::Dialect dialect = detail::Dialect::Jinja2, bool sandboxed = false)
        : m_saved(detail::t_parseLimits)
    {
        detail::t_parseLimits = { LimitOrNone(limits.maxExpressionDepth), LimitOrNone(limits.maxExpressionOperators), LimitOrNone(limits.maxBlockNesting), dialect, sandboxed };
    }
    ~ParseLimitsScope() { detail::t_parseLimits = m_saved; }
    ParseLimitsScope(const ParseLimitsScope&) = delete;
    ParseLimitsScope& operator=(const ParseLimitsScope&) = delete;
    ParseLimitsScope(ParseLimitsScope&&) = delete;
    ParseLimitsScope& operator=(ParseLimitsScope&&) = delete;

private:
    detail::ParseLimits m_saved;
};

namespace detail
{
[[noreturn]] void ThrowRenderMemory(std::uint64_t bytes);
// ChargeObjectMemory past the budget: notes the overdraft and makes the next step take the slow path, which fails it
// Exported: ValueObject's allocation function calls it from code outside a shared library
JINJA2CPP_EXPORT void OverdrawRenderMemory(std::uint64_t bytes) noexcept;
} // namespace detail

// Counts `bytes` of a string, list or dict the running render makes or grows against its memory budget
// (maxRenderMemory, docs/tasks/0459): throws ResourceLimitError when they do not fit. Without a budget one compare
inline void ChargeRenderMemory(std::uint64_t bytes)
{
    auto& limits = detail::t_renderLimits;
    if (limits.memoryLeft != UINT64_MAX)
    {
        if (bytes > limits.memoryLeft)
        {
            detail::ThrowRenderMemory(bytes);
        }
        limits.memoryLeft -= bytes;
    }
}
// Counts `bytes` of an object the running render makes (ValueObject's allocation function, docs/tasks/0478), where
// nothing but std::bad_alloc may be thrown: past the budget the render fails with maxRenderMemory at its next step.
// Without a budget one compare
inline void ChargeObjectMemory(std::uint64_t bytes) noexcept
{
    auto& limits = detail::t_renderLimits;
    if (limits.memoryLeft != UINT64_MAX)
    {
        if (bytes > limits.memoryLeft)
        {
            detail::OverdrawRenderMemory(bytes);
            return;
        }
        limits.memoryLeft -= bytes;
    }
}
// Throws ResourceLimitError when `bytes` would not fit in the running render's memory budget, without counting
// them: checked before a buffer is allocated that is counted once it is wrapped in a value
inline void CheckRenderMemoryFits(std::uint64_t bytes)
{
    if (bytes > detail::t_renderLimits.memoryLeft)
    {
        detail::ThrowRenderMemory(bytes);
    }
}
// Whether the running render has a memory budget: a container that counts its parts one by one asks first
inline bool RenderMemoryCounted() noexcept
{
    return detail::t_renderLimits.memoryLeft != UINT64_MAX;
}
// Gives back the bytes of one that is freed. Whichever render made it, the budget gains them (one made before the
// render or by another render frees memory too), up to the whole budget
inline void ReleaseRenderMemory(std::uint64_t bytes) noexcept
{
    auto& limits = detail::t_renderLimits;
    if (limits.memoryLeft != UINT64_MAX)
    {
        limits.memoryLeft = bytes < limits.memoryBudget - limits.memoryLeft ? limits.memoryLeft + bytes : limits.memoryBudget;
    }
}

namespace detail
{
void CountTemplateLoadSlow();
[[noreturn]] void ThrowRangeTooLong(std::uint64_t size);
} // namespace detail

// Counts one template that `include`, `extends` or `import` loads (maxTemplateLoads)
inline void CountTemplateLoad()
{
    if (detail::t_renderLimits.loadsLeft != UINT64_MAX)
    {
        detail::CountTemplateLoadSlow();
    }
}
// Throws ResourceLimitError when range() would have more than maxRange items
inline void CheckRangeSize(std::uint64_t size)
{
    if (size > detail::t_renderLimits.maxRange)
    {
        detail::ThrowRangeTooLong(size);
    }
}

// Whether the current thread has less stack left than one more level of parsing or
// rendering may need. False where the stack bounds are unknown. Called for every evaluated
// expression node, so the common case is one compare against a thread-local.
inline bool StackNearlyExhausted()
{
#if defined(_MSC_VER) && !defined(__clang__)
    const auto frame = reinterpret_cast<std::uintptr_t>(_AddressOfReturnAddress());
#else
    // The real frame even under ASan, which may move locals to a heap "fake stack"
    const auto frame = reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0));
#endif
    return frame < detail::t_threadStack.limit && detail::StackNearlyExhaustedSlow(frame);
}

// Counts one nesting level for as long as it lives
class DepthGuard
{
public:
    explicit DepthGuard(unsigned& depth)
        : m_depth(depth)
    {
        ++m_depth;
    }
    ~DepthGuard() { --m_depth; }
    DepthGuard(const DepthGuard&) = delete;
    DepthGuard& operator=(const DepthGuard&) = delete;
    DepthGuard(DepthGuard&&) = delete;
    DepthGuard& operator=(DepthGuard&&) = delete;

    [[nodiscard]] bool Exceeds(unsigned limit) const { return m_depth > limit; }

private:
    unsigned& m_depth;
};

// Counts the operators of sibling expressions (list items, call arguments) from the same
// base and leaves the counter at the deepest one when it goes
class SiblingOperators
{
public:
    explicit SiblingOperators(unsigned& counter)
        : m_counter(counter)
        , m_base(counter)
        , m_peak(counter)
    {
    }
    ~SiblingOperators() { m_counter = std::max(m_peak, m_counter); }
    SiblingOperators(const SiblingOperators&) = delete;
    SiblingOperators& operator=(const SiblingOperators&) = delete;
    SiblingOperators(SiblingOperators&&) = delete;
    SiblingOperators& operator=(SiblingOperators&&) = delete;

    // Before each sibling
    void Next()
    {
        m_peak = std::max(m_peak, m_counter);
        m_counter = m_base;
    }

private:
    unsigned& m_counter;
    unsigned m_base;
    unsigned m_peak;
};

// Depth of a left-associative operator chain (a + b + c): each operator is a level above
// the deeper of its operands, as in the tree the evaluator recurses through
class OperatorChain
{
public:
    explicit OperatorChain(unsigned& counter)
        : m_counter(counter)
        , m_base(counter)
    {
    }

    // Before the operand right of an operator: it starts from where the chain started
    void BeforeRight()
    {
        m_left = m_counter;
        m_counter = m_base;
    }
    // After it; false past the limit
    [[nodiscard]] bool AfterRight(unsigned limit)
    {
        m_counter = std::max(m_left, m_counter) + 1;
        return m_counter <= limit;
    }

    // After the operand of an operator that joins a flat run with the one before (`~` operands
    // are one node, not nested): the run is no deeper than its deepest operand
    [[nodiscard]] bool AfterRightFlat(unsigned limit)
    {
        m_counter = std::max(m_left, m_counter);
        return m_counter <= limit;
    }

private:
    unsigned& m_counter;
    unsigned m_base;
    unsigned m_left = 0;
};

// For as long as it lives, every CheckStack takes the slow path (RenderLimits::eachPiece): a render that writes
// each piece (RenderOptions::writeEachPiece) makes one, after its RenderLimitsScope
class EachPieceScope
{
public:
    EachPieceScope() noexcept
        : m_saved(detail::t_renderLimits.eachPiece)
    {
        detail::t_renderLimits.eachPiece = true;
        detail::t_threadStack.renderLimit = UINTPTR_MAX;
    }
    ~EachPieceScope()
    {
        auto& limits = detail::t_renderLimits;
        limits.eachPiece = m_saved;
        auto& stack = detail::t_threadStack;
        stack.renderLimit = limits.active || limits.eachPiece ? UINTPTR_MAX : stack.limit;
    }
    EachPieceScope(const EachPieceScope&) = delete;
    EachPieceScope& operator=(const EachPieceScope&) = delete;
    EachPieceScope(EachPieceScope&&) = delete;
    EachPieceScope& operator=(EachPieceScope&&) = delete;

private:
    bool m_saved;
};

// Throws RecursionLimitError when the thread's stack is nearly used up. The renderer's and
// the evaluator's check: in a render with limits each call is a render step, which may throw
// ResourceLimitError
inline void CheckStack()
{
#if defined(_MSC_VER) && !defined(__clang__)
    const auto frame = reinterpret_cast<std::uintptr_t>(_AddressOfReturnAddress());
#else
    const auto frame = reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0));
#endif
    if (frame < detail::t_threadStack.renderLimit)
    {
        detail::CheckStackSlow(frame);
    }
}

// CheckStack, telling whether it took the slow path: only then can a render that writes each piece run
// (EachPieceScope), so a body reads its stream's flag there and the usual render tests nothing more
[[nodiscard]] inline bool CheckStackTookSlowPath()
{
#if defined(_MSC_VER) && !defined(__clang__)
    const auto frame = reinterpret_cast<std::uintptr_t>(_AddressOfReturnAddress());
#else
    const auto frame = reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0));
#endif
    if (frame < detail::t_threadStack.renderLimit)
    {
        detail::CheckStackSlow(frame);
        return true;
    }
    return false;
}

// One level of render nesting; throws RecursionLimitError past
// Settings::Limits::maxRenderDepth or when the stack runs low. The count is per thread, so it
// spans every template and context one render goes through.
class RenderDepthGuard
{
public:
    RenderDepthGuard()
        : m_guard(Depth())
    {
        if (m_guard.Exceeds(detail::t_renderLimits.maxRenderDepth))
        {
            throw RecursionLimitError();
        }
        CheckStack();
    }

private:
    // A suspendable render keeps a count of its own (detail::RenderThreadState, docs/tasks/0457)
    friend struct detail::RenderThreadState;
    static unsigned& Depth()
    {
        thread_local unsigned depth = 0;
        return depth;
    }

    DepthGuard m_guard;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_RECURSION_GUARD_H
