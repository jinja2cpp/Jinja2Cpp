---
status: open
priority: low
area: parity
depends: [0049]
touches: [src/template_module.cpp, src/template_impl.h]
shares: [src/render_context.h, src/cycle_collector.cpp]
---
# A macro a module call stores keeps its module, as in Jinja2

**Problem.** Since 0301 a `Template::MakeModule` call ends the modules it imports with
context. A macro of such a module that the call stored (`{% set ns.f = a.am %}`, or
`a.am.__call__`) fails with `TemplateExpired` when a later call runs it. Jinja2 keeps the
module while something reaches the macro and prints its output (3.1.6 prints `AM` for
`TemplateModuleTest.MacroStoredAcrossCallsExpires`).

**Proposal.** At the end of a call, walk what the call stored into (the collector's noted
receivers) with 0049's `Traverse` hook for closures homed in a module about to end, and keep
those modules; release a kept module once no stored closure reaches it, so that a macro
stored again on every call does not grow the module. Plan section 3, option C, of
/mnt/project-files/release-2.0/0301-plan.md compares this with a refcount from closure to
module.

**Done when** `MacroStoredAcrossCallsExpires` prints `AM` for both stores, the 0301
steady-state test still holds with a macro stored on every call, and Render stays within
the 0038 gates.
