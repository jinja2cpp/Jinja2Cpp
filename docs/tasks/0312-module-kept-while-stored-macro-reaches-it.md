---
status: in-progress
priority: low
area: parity
depends: [0049]
touches: [src/template_module.cpp, src/template_impl.h, src/statements.cpp, src/statements.h, test/template_module_test.cpp, test/allocation_failure_test.cpp]
shares: [src/render_context.h, src/cycle_collector.h]
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

**Resolution (in progress).** Not option C's walk from the noted receivers, which cannot tell a receiver only the ending
module holds (its own namespace) from one that lives on, nor a refcount from closure to module, which reopens the cycle
problem. `RendererCallback::EndCall` of the outermost call now decides which modules it ends:

- The candidates are the modules the call made that are neither shared nor imported by a shared one's top level, and
  those an earlier walk kept (`storedKept`).
- When a call since the last walk noted something (a store, a container letting go of a value: the collector has
  notes, `CycleCollector::HasNotes`), `ImportedModule::MarkReached` walks from what lives on, the template module's
  own values (`AppendHeldObjects` of `TemplateModuleState::module`) and the modules that stay, through
  `ValueObject::Traverse`. A candidate whose closure it finds (a macro, a caller, `super`, a block of `self`, whose
  `ClosureSite::RunsInEndingModule` names the candidate's context) stays, and the walk goes on from what that module
  holds, so that a module it imported with context stays too. The rest end, newest first, as before.
- Without notes nothing that lives on can have changed: the new modules end and the kept ones stay. A nested call
  leaves its modules to the outermost call once something was noted, since its caller's frames may hold them.
- When the walk cannot allocate, the kept modules stay and the call's modules end, as before 0312.

Failure is the safe direction: a closure the walk does not see (behind a `std::function` capture or a host's map, or
held only by the host, such as a frame copy that `KeptObjects` holds) leaves its module to end, and a call of it fails
with `TemplateExpired` as before; nothing only a dying module holds can keep it, so a module never stays for nothing
(`ModuleCallTest.StoredMacrosStayFlat`, whose rows include a module holding its own macro in a namespace that holds
itself).

**Cost.** The walk visits what the template module's values reach, on the outermost calls that noted something while
a module the call made, or one kept so, is a candidate. A module that both keeps a large state and imports with
context on every call pays a walk of that state per call; Render and calls that import nothing with context pay a flag
test.
