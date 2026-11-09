#ifndef JINJA2CPP_PYTHON_CONTEXT_H
#define JINJA2CPP_PYTHON_CONTEXT_H

#include "common.h"

#include <memory>
#include <string>

namespace jinja2
{
inline namespace v2
{
class RenderContext;
}
} // namespace jinja2

namespace jinja2cpp_py
{
struct CallLease;

// What a callable asks for as its first argument: Jinja2's pass_context, pass_eval_context and
// pass_environment, which mark a function with `jinja_pass_arg` (jinja2.utils._PassArg)
enum class PassArg : unsigned char
{
    None,
    Context,
    EvalContext,
    Environment
};

// The marker of `callable`, or of its __call__, as Jinja2's Context.call reads it
PassArg PassArgOf(PyObject* callable);

// The extra first argument of a call that asked for one, made for the render `context` runs in:
// the environment of the render, an EvalContext, or a read-only Context lent for the call (the
// innermost LeaseScope), which raises TemplateRuntimeError once the call has returned
nb::object MakePassArgument(PassArg kind, jinja2::RenderContext& context);

// The part of a Context that reads the render (jinja2cpp.runtime.Context holds one): lent for the
// duration of the call that received it
class ContextHandle
{
public:
    explicit ContextHandle(std::shared_ptr<CallLease> lease);

    // The value of `name` the template sees at the call, or None with `found` false
    nb::object Lookup(const std::string& name, bool& found) const;
    // The names the template's scopes and render arguments hold at the call
    nb::list Names() const;

private:
    jinja2::RenderContext& Live() const;

    std::shared_ptr<CallLease> m_lease;
};

void BindContext(nb::module_& module);
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_CONTEXT_H
