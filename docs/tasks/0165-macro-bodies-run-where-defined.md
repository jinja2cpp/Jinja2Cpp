---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/508
priority: high
area: parity
depends: []
touches: [src/render_context.h, src/render_workspace.h, src/render_workspace.cpp, src/slot_frame.h, src/statements.cpp#macro, src/statements.cpp#MacroCallStatement, src/statements.cpp#MakeLoopRecursion]
shares: [src/statements.h, src/template_parser.cpp, test/parity/cases/statements.py, test/parity/cases/loader.py, test/macro_test.cpp, test/helpers_tests.cpp]
---
# Macro and call bodies run where they were defined

**Problem.** A macro body, a call body (`caller`), a recursive loop's `loop()` and
`super()` resolve free names through whoever calls them (dynamic scoping), so a macro sees
its caller's locals and loop targets, and a call body sees the macro's loop target instead
of its own definition site. Python resolves them where they are defined, live. The same
mismatch breaks a call block on a macro imported without context (`<><>` where Python gives
`<1><2>`, a common forms-library pattern) and a macro passed into an imported macro.

**Proposal.** Phase PR 1 of the approved plan, docs/design/0038-lexical-scoping-plan.md
(sections 4.2, 4.4 and 7, as amended by sections 10 and 11). At definition each closure
records a `ClosureSite` (an opaque class with `Resolve()`): a `weak_ptr<ContextAnchor>` to
the defining context, the scope depth and serial there, the template root depth and a
closure epoch with the context's `forgetGeneration`. A call locks the anchor for its
duration only and runs the body on the home context through a full-RAII `HomeCall`, in a
scope whose `linkBelow` skips every scope the caller pushed above the definition point.
Ownership (section 11): the context is the only owner of its anchor (`~RenderContext`
asserts `use_count() == 1`); nothing else holds a context strongly. A closure that outlives
its frame reads that frame's names as undefined (deliberate divergence lines listed in
section 7). Rewrite the invariant comments at render_context.h around the child-prefix
rule and add the child-side Debug assert.

**Done when.** No line of `test/parity/divergences/` names 0165, the new corpus rows of
plan section 7 (PR 1) and section 10 point 6 (recursion via `with`/include) are added with
their deliberate lines, `ctest -R parity` passes, the verifier and the sanitizer
configuration are clean, and the Performance track's verdict holds: Load ≤ +0.5%, Render
±0.5% (named exception allowed for `macros` up to +0.7%, only if measured, decision 8).
Changelog entry for the behaviour change: pass a caller's loop variable as an argument.

**Next.** 0166 (defaults per call) builds on the home call; 0167 runs module macros through it.
Sequence 0164 after this task (both edit render_context.h and render_workspace.*); 0143
cannot share a wave with it.

**Progress.** Implemented on the 0165 branch: `ClosureSite`/`HomeCall` (render_context.h),
`ScopeView::linkBelow`, lazily stamped scope serials, link-aware lookups; macros, `caller`,
recursive `loop()` and `super()` capture their sites. `loop()` now checks its arguments as
`LoopContext.__call__(iterable)`. Deviations from the plan: `forgetGeneration` is replaced
by comparing the home's epoch at the call with the one the closure recorded; top-level
sites are recognised at run time (`TemplateFrame::baseDepth == depth`); a running site
forces a fresh epoch for recursion. Instruction counts (bench/count.py, Release, against
dd9dd3d): every Load within ±0.5%; Render macros +0.66% (the named exception), inheritance
+0.39%, config_file +0.37%, plain_text +0.40% (4 instructions), strings +0.30%,
html_autoescape +0.28%, the rest within ±0.2%. A top-level macro called inside a loop or
another body takes its own lookup cache epoch (its link hides that body's scopes, whose
names the home's epoch may have cached), unless the Load marked its body as reading only
its arguments' slots (MacroStatement::ReadsSlotsOnly, conservative: no call, filter, test,
attribute access, statement other than `if`, nor name outside its arguments).

Done in #508 (merged a6e9573, 2026-10-13): CI green on all 41 checks, Render macros
+0.69% against the post-train master, inside the +0.7% allowance. The lines that still name
0165 are its `deliberate:` lines, which the umbrella 0038 accepts.
