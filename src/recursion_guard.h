#ifndef JINJA2CPP_SRC_RECURSION_GUARD_H
#define JINJA2CPP_SRC_RECURSION_GUARD_H

#include <jinja2cpp/template_env.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <exception>
#include <string>

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

namespace jinja2
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
// maxBlockNesting, which the parser reads, and maxRenderDepth, which RenderDepthGuard reads
// from the render's RenderLimits.

namespace detail
{
// The current thread's stack below `limit` is the reserve that StackNearlyExhausted keeps
// free; `low` is its end. The bounds are read on the first check in a thread, which always
// takes the slow path; a stack of unknown bounds gets limit 0 and is never checked again.
struct ThreadStack
{
    std::uintptr_t limit = UINTPTR_MAX;
    std::uintptr_t low = 0;
};
inline thread_local ThreadStack t_threadStack;

bool StackNearlyExhaustedSlow(std::uintptr_t frame);

// What the running render may still do (Settings::Limits, docs/tasks/0262). A limit that is
// off holds the largest value of its type, so each check is one compare. Outside a render,
// and in a render without limits, every limit is off.
struct RenderLimits
{
    // Steps left before the slow path (StepsSlow): the rest of the step budget, or of the 1024
    // steps between two clock polls, whichever ends first
    std::uint64_t stepsLeft = UINT64_MAX;
    // Steps the countdown started from, and the steps taken before it
    std::uint64_t chunk = UINT64_MAX;
    std::uint64_t stepsTaken = 0;
    std::uint64_t maxSteps = 0;
    // The built-in cap (MaxSequenceSize in internal_value.h) or a lower maxSequenceSize
    std::uint64_t maxSequenceSize = std::uint64_t{ 1 } << 26;
    std::uint64_t maxRange = UINT64_MAX;
    std::uint32_t loadsLeft = UINT32_MAX;
    unsigned maxRenderDepth = Settings::Limits{}.maxRenderDepth;
    bool hasDeadline = false;
    std::chrono::steady_clock::time_point deadline{};
};
// Constant-initialised, so that reading it needs no guard
inline thread_local RenderLimits t_renderLimits;

void StepsSlow();
void PollDeadline();
} // namespace detail

// A limit of Settings::Limits that a render passed. Template rendering reports it as
// ErrorCode::ResourceLimitExceeded with the message and the field's name
class ResourceLimitError : public std::exception
{
public:
    ResourceLimitError(const char* field, std::string message)
        : m_field(field)
        , m_message(std::move(message))
    {
    }
    [[nodiscard]] const char* what() const noexcept override { return m_message.c_str(); }
    [[nodiscard]] const char* Field() const noexcept { return m_field; }

private:
    const char* m_field;
    std::string m_message;
};

// Installs a render's limits for as long as it lives and puts back the ones of the render it
// interrupts (a host callable may render another template)
class RenderLimitsScope
{
public:
    explicit RenderLimitsScope(const Settings::Limits& limits);
    ~RenderLimitsScope() { detail::t_renderLimits = m_saved; }
    RenderLimitsScope(const RenderLimitsScope&) = delete;
    RenderLimitsScope& operator=(const RenderLimitsScope&) = delete;
    RenderLimitsScope(RenderLimitsScope&&) = delete;
    RenderLimitsScope& operator=(RenderLimitsScope&&) = delete;

private:
    detail::RenderLimits m_saved;
};

// Counts one render step (Settings::Limits::maxRenderSteps): a statement body, an item a loop
// filter looks at, a macro or host callable call. Every 1024 steps it polls the deadline
inline void CountStep()
{
    if (--detail::t_renderLimits.stepsLeft == 0)
    {
        detail::StepsSlow();
    }
}

// After a host callable returns: a render past its deadline stops here, not 1024 steps later
inline void AfterHostCall()
{
    if (detail::t_renderLimits.hasDeadline)
    {
        detail::PollDeadline();
    }
}

// Counts one template that `include`, `extends` or `import` loads (maxTemplateLoads)
void CountTemplateLoad();
// Throws ResourceLimitError when range() would have more than maxRange items
void CheckRangeSize(std::uint64_t size);

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

// A depth limit of Settings::Limits as the counters compare it: 0, no limit, never passed
constexpr unsigned LimitOrNone(std::uint32_t limit)
{
    return limit != 0 ? limit : UINT_MAX;
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

// Python's RecursionError. Template rendering reports it as ErrorCode::RecursionLimitExceeded
class RecursionLimitError : public std::exception
{
public:
    [[nodiscard]] const char* what() const noexcept override { return "maximum recursion depth exceeded"; }
};

// Throws RecursionLimitError when the thread's stack is nearly used up
inline void CheckStack()
{
    if (StackNearlyExhausted())
    {
        throw RecursionLimitError();
    }
}

// One level of render nesting, which is a render step too; throws RecursionLimitError past
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
        CountStep();
    }

private:
    static unsigned& Depth()
    {
        thread_local unsigned depth = 0;
        return depth;
    }

    DepthGuard m_guard;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_RECURSION_GUARD_H
