#ifndef JINJA2CPP_SRC_RECURSION_GUARD_H
#define JINJA2CPP_SRC_RECURSION_GUARD_H

#include <algorithm>
#include <exception>

namespace jinja2
{

// Nesting limits that keep hostile or runaway templates from overflowing the stack
// (docs/tasks/0003). They are sized for a 1 MiB stack (the Windows main thread; macOS
// secondary threads have 512 KiB) in Debug builds, and stay near where Python Jinja2 stops
// the same templates with RecursionError or SyntaxError.
//
// Expression nesting: brackets, calls, subscripts, filter arguments. Python Jinja2 fails at
// about 80 nested brackets; each level costs Jinja2C++ some 10 KiB of parser stack.
constexpr unsigned MaxExpressionDepth = 64;
// Operators chained on one path of an expression: the parser reads a + b + c, x|f|g, a.b.c
// and - - x in a loop, but the evaluator recurses once per operator. Siblings (list items,
// call arguments) do not add up; the deepest one counts. Python fails at 200-300.
constexpr unsigned MaxExpressionOperators = 256;
// Render nesting: macro and caller() calls, super() and self.<block>, loop() of recursive
// loops, included, imported and parent templates. Python fails at about 250 macro calls.
constexpr unsigned MaxRenderDepth = 256;
// Statement blocks open at once ({% if %}, {% for %}, {% macro %}, ...), each a level of
// render recursion. Python compiles a template to Python code and fails at about 100 nested
// blocks (IndentationError), 20 of them loops.
constexpr unsigned MaxBlockNesting = 128;

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

// Python's RecursionError. Template rendering reports it as ErrorCode::RecursionLimitExceeded
class RecursionLimitError : public std::exception
{
public:
    [[nodiscard]] const char* what() const noexcept override { return "maximum recursion depth exceeded"; }
};

// One level of render nesting; throws RecursionLimitError past MaxRenderDepth. The count
// is per thread, so it spans every template and context one render goes through.
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
