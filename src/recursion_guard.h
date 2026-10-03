#ifndef JINJA2CPP_SRC_RECURSION_GUARD_H
#define JINJA2CPP_SRC_RECURSION_GUARD_H

#include <algorithm>
#include <cstdint>
#include <exception>

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
// Expression nesting: brackets, calls, subscripts, filter arguments. Python Jinja2 fails at
// about 70 nested brackets.
constexpr unsigned MaxExpressionDepth = 64;
// Operator depth of an expression: a + b + c, x|f|g, a.b.c, - - x and a if x else b if y
// else c nest one level per operator although the parser reads them in a loop. Siblings
// (operands, list items, call arguments) do not add up; the deepest one counts. Python
// fails at 300-500.
constexpr unsigned MaxExpressionOperators = 400;
// Render nesting: macro and caller() calls, super() and self.<block>, loop() of recursive
// loops, included, imported and parent templates. Python fails at about 250 macro calls.
constexpr unsigned MaxRenderDepth = 256;
// Statement blocks open at once ({% if %}, {% for %}, {% macro %}, ...; elif and else do
// not count). Python compiles a template to Python code and fails at about 100 nested
// blocks (IndentationError), 20 of them loops.
constexpr unsigned MaxBlockNesting = 128;

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
} // namespace detail

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

// One level of render nesting; throws RecursionLimitError past MaxRenderDepth or when the
// stack runs low. The count is per thread, so it spans every template and context one
// render goes through.
class RenderDepthGuard
{
public:
    RenderDepthGuard()
        : m_guard(Depth())
    {
        if (m_guard.Exceeds(MaxRenderDepth))
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

} // namespace jinja2

#endif // JINJA2CPP_SRC_RECURSION_GUARD_H
