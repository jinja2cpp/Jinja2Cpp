---
status: open
priority: low
area: parity
depends: [0326, 0323]
touches: [src/statements.cpp, src/render_context.h, src/template_module.cpp, src/cycle_collector.cpp, test/parity/divergences/loader.txt, test/parity/divergences/statements.txt]
---
# Closures kept past a module call or a scoped frame lose their names

**Problem.** Found by the verifier of parity batch 2 group C (0326, 0323). A closure that keeps
the names of the frame it was defined in (`ClosureCapture`) loses them in five cases, and one
case holds memory until the render ends.

1. *A host's module call.* The copies made during a call the host makes into a template module
   (`TemplateModuleState::Call`) are owned by that call's cycle collector, which releases them
   when the call ends. A closure that a module macro stores in module state then reads the
   macro's names as undefined. Module:

   ```jinja
   {% set ns = namespace(f=none) %}{% macro keep(n) %}{% macro i() %}I{{ n }}{% endmacro %}{% set ns.f = i %}{% endmacro %}{% macro later() %}{{ ns.f() }}{% endmacro %}{{ keep('top') }}
   ```

   `later()` from the host gives `I`, and after the host calls `keep('host')` still `I`.
   Python Jinja2 3.1.6 gives `Itop`, then `Ihost`. No corpus case: the corpus renders
   templates, not host calls into a module; pin it in `test/template_module_test.cpp`.
2. *A `with` or `block` frame inside an included template* (a `with` inside a macro call too:
   `|4missing|4missing` in Python, `||` here). The include captures its context
   when it ends (`RenderIncludedTemplate`), but a closure defined in a `with` or `block` body
   there registered with the frame that ended first, so nothing is copied for it: the block
   case reads `a` and `q` as undefined (`|`, Python `|AQ`), the `with` case reads `q` as
   undefined too (`|missing`, Python `|missingQ`). Corpus rows
   `loader.include_block_macro_kept` and `loader.include_with_macro_kept`.
3. *A loop frame inside a macro call.* A macro defined in a `for` body of a macro call and kept
   past the call reads the call's names as undefined too, not only the loop's (Jinja2 prints its
   `missing` sentinel for the loop variable): `{% macro outer(v) %}{% for j in [1, 2] %}{% macro
   m() %}{{ j }}{{ v }}{% endmacro %}{% set ns.f = m %}{% endfor %}{% endmacro %}` gives `` for
   `ns.f()` after `outer(7)`, Python `missing7`. Corpus row
   `statements.macro_closure_in_loop_of_call`. The loop's capture is skipped because the loop's
   scope has ended when the call captures.
5. *`self` of an ended include.* A macro of an included template kept past the include and
   calling `self.<block>` fails: the `self` its call finds is rebuilt without the blocks
   (`ns.f()` with `{% macro a() %}a{{ y }}{{ self.blk() }}{% endmacro %}{% block blk %}...`
   in the include: Python `aYBY`, an undefined-attribute error here, at base 9fad25c too).
   Corpus row `loader.include_macro_kept_reads_self`.
6. *An enclosing frame that still runs.* A nested frame's copy reaches down to its template's
   top level, so it also copies the enclosing macro call or loop that is still running: a
   closure called later in that call reads the values as they were when the nested frame ended
   (`{% macro outer() %}{% set y = 1 %}{% macro k() %}{% macro c() %}{{ y }}{% endmacro %}{% set ns.f = c %}{% endmacro %}{{ k() }}{% set y = 2 %}{{ ns.f() }}{% endmacro %}`:
   Python `2`, here `1`), and a macro the enclosing frame defines after it reads as undefined.
   The include top level reads live since 0326 (one copy at the include's end, the base);
   the same chain is needed for enclosing frames: copy each frame's own scopes, and let the
   enclosing frame's copy become the base when it ends. Corpus rows
   `statements.macro_closure_enclosing_call_live`, `statements.macro_closure_enclosing_loop_live`.
4. *Memory.* A closure stored in a list, dict or namespace made in its own frame
   (`{% set lst = [] %}{% macro m() %}{{ lst|length }}{% endmacro %}{% set _ = lst.append(m) %}`)
   closes a cycle through the copy of the frame (copy, container, closure, capture), which the
   render frees only when it ends, as it does every reference cycle (0110): a loop of 100k such
   calls peaks at about 145 MB in a Debug build, Python at 22 MB. A plain cycle
   (`{% set l = [] %}{% set _ = l.append(l) %}` in a macro) grows the same way, by less per call.

**Proposal.** (1) Own the copies of a module call by the module's state (or the render that
holds it) rather than by the call's collector, so they live as long as the closure can be
reached. (2, 3) Capture a closure's frame at the end of the scoped frame it was defined in (the
`with`, block, scoped block or loop iteration), as `InvokeMacroRenderer` does for a macro call,
and let the enclosing frame's capture add its names when it ends. (4) Collect the cycles of a
render while it runs, as Python's generational collector does, or let the collector see a
closure's capture and copy (ValueObject::Traverse) so a walk run when the copies pass a count
frees the dead ones.

**Done when.** The corpus rows above match and their divergence lines are deleted, a module-state
test pins case 1, and the memory bound of 0326 (copies freed once no closure outside them
holds the capture) still holds, and the case 4 loop stays in constant memory.
