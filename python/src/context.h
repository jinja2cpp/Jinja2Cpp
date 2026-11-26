#ifndef JINJA2CPP_PYTHON_CONTEXT_H
#define JINJA2CPP_PYTHON_CONTEXT_H

#include "common.h"

#include <jinja2cpp/value.h>

#include <array>
#include <memory>
#include <optional>
#include <string>

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

// The extra first argument of a call that asked for one, made for the innermost call the engine makes
// (LeaseScope): the environment of the render, an EvalContext, or a read-only Context lent for the
// call, which raises TemplateRuntimeError once the call has returned
nb::object MakePassArgument(PassArg kind);

// The part of a Context that reads the render (jinja2cpp.runtime.Context holds one): lent for the
// duration of the call that received it
class ContextHandle
{
public:
    explicit ContextHandle(std::shared_ptr<CallLease> lease);

    // Which names a lookup sees: all those the template sees at the call, or those of Jinja2's Context.vars (the
    // template's top level) or Context.parent (the render arguments and globals), or the names a `set` stored so far
    // in the loop or block body the call is written in (Jinja2's _loop_vars and _block_vars), jinja2::ContextView; or,
    // for a lookup only, Jinja2's resolve: those of the body, then of vars, then of parent
    enum class Part
    {
        All,
        Vars,
        Parent,
        Body,
        Resolve,
    };
    // The value of `name` the template sees at the call among `part`, or None with `found` false
    nb::object Lookup(const std::string& name, Part part, bool& found) const;
    // The names of `part` at the call
    nb::list Names(Part part) const;

private:
    // The context, or a view of it, for `part`
    [[nodiscard]] jinja2::Value View(Part part) const;
    // The template's context (UserCallableParams::context), or TemplateRuntimeError after the call
    const jinja2::GenericMap& Live() const;

    std::shared_ptr<CallLease> m_lease;
    // The views of Vars, Parent and Body, made at their first read: a view reads the render as it goes and expires
    // with the call, so one per part serves every lookup of the call (each view allocates its mapper)
    mutable std::array<std::optional<jinja2::Value>, 3> m_views;
};

void BindContext(nb::module_& module);
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_CONTEXT_H
