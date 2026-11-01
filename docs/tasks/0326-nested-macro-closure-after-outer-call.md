---
status: in-progress
priority: medium
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
level, which it still reads live. The render owns each copy while a closure outside it still holds its
capture (`CycleCollector::KeepUntilEnd`, which prunes the others and releases the rest before
the cycle walk); the site holds the copy weakly, so a namespace that stores the closure and is
among the names copied forms no cycle. A closure among its own copied names goes in weakened
(`MacroStatement::WeakenCapture`): it still calls itself and its siblings, without keeping the
copy alive, and is `sameas` the macro it copies (both compare by the capture). A macro whose body reads no names keeps no copy. 200k nested-macro calls or 100k
includes in a loop peak at 13 MB (180 MB before the pruning). A macro defined in a loop and kept past
the loop still reads the loop's names as undefined (Jinja2 prints its `missing` sentinel there;
the deliberate lines stay). Also fixed: `statements.nested_macro_escapes_its_call` (0165's
deliberate line). Corpus rows `statements.macro_closure_*`.

Limits, filed as 0373: copies made in a host's module call die with that call, and a closure
defined in a `with` or block of an included template keeps nothing of it
(`loader.include_block_macro_kept`, `loader.include_with_macro_kept`).
