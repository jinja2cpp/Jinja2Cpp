#ifndef JINJA2CPP_SRC_RECURSION_GUARD_H
#define JINJA2CPP_SRC_RECURSION_GUARD_H

#include "engine_errors.h"

#include <jinja2cpp/big_int_backend.h>
#include <jinja2cpp/template_env.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <exception>
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
inline thread_local ThreadStack t_threadStack;

bool StackNearlyExhaustedSlow(std::uintptr_t frame);

// The parser's limits for the template being loaded (ParseLimitsScope), in a thread-local
// rather than in each parser, which lives for one tag
struct ParseLimits
{
    unsigned maxExpressionDepth = 70;
    unsigned maxExpressionOperators = 491;
    unsigned maxBlockNesting = 248;
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
    // Whether a render installed these, for its limits or its big-integer backend: a render
    // without either inside one with them installs the defaults
    bool scoped = false;
    bool hasDeadline = false;
    // steady_clock ticks since its epoch: a time_point's constructor is not noexcept, which a
    // thread-local's initialiser has to be
    std::chrono::steady_clock::rep deadline = 0;
};
static_assert(RenderLimits{}.maxRenderDepth == Settings::Limits{}.maxRenderDepth);
// Constant-initialised, so that reading it needs no guard
inline thread_local RenderLimits t_renderLimits;

void PollDeadline();
void InstallRenderLimits(const RenderLimits& limits);
} // namespace detail

// Installs a render's limits for as long as it lives and puts back the ones of the render it
// interrupts (a host callable may render another template). A render without limits (null)
// on a thread that runs none costs a test of a thread-local flag
class RenderLimitsScope
{
public:
    // `limits` is null when the render has neither limits nor a big-integer backend
    RenderLimitsScope(const Settings::Limits* limits, const BigIntBackend* bigInts)
    {
        if (limits || detail::t_renderLimits.scoped)
        {
            Install(limits, bigInts);
        }
    }
    ~RenderLimitsScope()
    {
        if (m_saved)
        {
            detail::InstallRenderLimits(*m_saved);
        }
    }
    RenderLimitsScope(const RenderLimitsScope&) = delete;
    RenderLimitsScope& operator=(const RenderLimitsScope&) = delete;
    RenderLimitsScope(RenderLimitsScope&&) = delete;
    RenderLimitsScope& operator=(RenderLimitsScope&&) = delete;

private:
    void Install(const Settings::Limits* limits, const BigIntBackend* bigInts);

    std::optional<detail::RenderLimits> m_saved;
};

// Whether a template loaded with these limits renders with any (RenderLimitsScope): the
// Load-time limits aside, one of them differs from the defaults
inline bool HasRenderLimits(const Settings::Limits& limits)
{
    const Settings::Limits defaults;
    return limits.maxRange != 0 || limits.maxSequenceSize != 0 || limits.maxOutputSize != 0 || limits.maxTemplateLoads != 0
           || limits.maxRenderSteps != 0 || limits.maxRenderTime.count() > 0 || limits.maxRenderDepth != defaults.maxRenderDepth;
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
    explicit ParseLimitsScope(const Settings::Limits& limits)
        : m_saved(detail::t_parseLimits)
    {
        detail::t_parseLimits = { LimitOrNone(limits.maxExpressionDepth), LimitOrNone(limits.maxExpressionOperators), LimitOrNone(limits.maxBlockNesting) };
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

private:
    unsigned& m_counter;
    unsigned m_base;
    unsigned m_left = 0;
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
