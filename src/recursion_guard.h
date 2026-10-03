#ifndef JINJA2CPP_SRC_RECURSION_GUARD_H
#define JINJA2CPP_SRC_RECURSION_GUARD_H

#include <exception>

namespace jinja2
{

// Nesting limits that keep hostile or runaway templates from overflowing the stack
// (docs/tasks/0003). Python Jinja2 stops the same templates with RecursionError, at about
// 90 nested brackets and 250 nested macro calls, so neither limit rejects a template
// Jinja2 renders.
//
// Expression nesting: brackets, calls, subscripts and chains of unary operators, counted
// by the recursive descent parser.
constexpr unsigned MaxExpressionDepth = 256;
// Operators chained in one statement: the parser reads a + b + c, x|f|g or a.b.c in a
// loop, but the evaluator recurses once per operator. Python Jinja2 fails at about 1000.
constexpr unsigned MaxExpressionOperators = 1024;
// Render nesting: macro and caller() calls, super() and self.<block>, recursive loops,
// included, imported and parent templates.
constexpr unsigned MaxRenderDepth = 256;

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
