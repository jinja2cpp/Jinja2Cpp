---
status: done
priority: medium
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/733
area: parity
touches: [src/statements.cpp, test/parity/divergences/statements.txt, src/render_context.h, src/cycle_collector.h, src/cycle_collector.cpp, test/parity/cases/statements.py]
---
# A macro defined in a macro loses the outer call's names after that call returned

**Problem.** Found by the 0223 scenario suite (row F5). A macro defined inside another macro and
stored outside it (a namespace attribute) reads the outer macro's parameters and locals as
undefined once the outer call has returned:

```jinja
{% set ns = namespace(f=none) %}{% macro outer(n) %}{% macro inner(k) %}{{ n }}{{ k }}{% endmacro %}{% set ns.f = inner %}{% endmacro %}{{ outer('captured') }}|{{ ns.f('k') }}
```

Python Jinja2 3.1.6 prints `|capturedk`; Jinja2C++ prints `|k`. A top-level name read from such a
closure works, and so does a stored `caller` (0165). Pinned by
`statements.macro_closure_outlives_outer_call` in the corpus.

**Proposal.** The closure keeps the frame of the call that defined it, as 0165 did for macro
bodies run where they are defined. Check against 0301 (D2a gives a macro stored in a namespace
and called in a later call `TemplateExpired`; this case is called after the call, in the same
render) and 0049 P1's traversal of closures.

**Done when.** The corpus case matches and its divergence line is deleted; ASan clean.

**Fix (parity batch 2, group C).** A macro defined in a frame inside its template's top level
registers a `ClosureCapture` with the context it is defined in (`ClosureSite::CaptureOnEnd`). When
a macro call ends (`InvokeMacroRenderer`), the context copies, for each closure defined in the
call's scope, the names that closure sees from there down to its template's top level
(`RenderContext::CaptureClosures`), slots included. A later call that finds the frame ended
(`ClosureSite::Resolve`, case 3) pushes the copy as a scope below the body, linked to the top
level, which it still reads live. The closures of one frame share one capture
and so one copy, in which each finds the others. The callable keeps its capture
(`Callable::SetKeepAlive`), the site only weakly, and the capture holds the copy weakly; the
render owns the copy while a closure outside it still holds the capture
(`CycleCollector::KeepUntilEnd`, which prunes the others and releases the rest before the cycle
walk). The copy holds the frame's own closures as copies of the same callables without the
keep-alive (`MacroStatement::WeakenCapture`), so it never holds itself; a call that reads the
copy gives the keep-alive back (`StrengthenCapture`), so recursion, sibling calls and a closure
stored from a kept one all work, and all of them stay `sameas` the macro they copy (one function
holder per definition). A frame end makes no copy when only the frame's own values hold its
closures (`RenderContext::CaptureEscapes`, by value box and `use_count`; a frame holding more than 32 closure
boxes is copied as if one escaped), and a macro whose body
reads no names keeps none: a frame whose nested macros never escape pays one capture per
definition. A nested frame of an included template copies only its own scopes and reads the
include's top level live; when the include ends, one copy of its top level and of what it saw of
the includer becomes the base of every capture of that context (`ClosureCapture::base`); a top-level
capture that only the base holds lives as long as the base.
200k nested-macro calls or 100k includes in a loop stay at about 14 MB (180 MB in the first
version). A module macro's result is copied without building a `std::function` per call
(`ExportCopier`'s wrap is a function pointer). Corpus rows `statements.macro_closure_*`.

Limits, filed as 0373: copies made in a host's module call die with that call; a closure
defined in a `with` or block of an included template, or in a loop of a macro call, keeps
nothing of its frames (`loader.include_block_macro_kept`, `loader.include_with_macro_kept`,
`statements.macro_closure_in_loop_of_call`); a kept include macro calling `self.<block>`
(`loader.include_macro_kept_reads_self`); a nested frame's copy of an enclosing call or loop
that still runs (`statements.macro_closure_enclosing_*_live`); a closure stored in a container made in its own frame
forms a cycle the render frees only when it ends, as every reference cycle (0110). Copying only
the names a closure reads, as Python's closures do, is 0375.
