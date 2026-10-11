---
status: done
priority: low
area: perf
depends: [0140]
touches: [src/expression_evaluator.h, src/expression_evaluator.cpp]
---
# Template constants pay an atomic refcount on every copy

**Problem.** After 0140 P2 a heap value (string, list, callable) is copied by one atomic
increment. A template's own constants are copied the most and shared by every thread that
renders the template, so their count is the most contended one. Yet the template outlives
every render that reads them.

**Proposal.** A "static" heap kind for `ConstantExpression` values (0118 P5c) that skips the
refcount. Before it can ship, a static value must turn into a refcounted copy wherever it can
outlive the template: `CreateGenericList`/`CreateGenericMap` capture `adapter = *this`
(internal_value.cpp:1048-1051) and reach user callables, so the host can keep a value after
the Template is destroyed. TemplateExpired does not cover that path.

**Done when.** Measured after 0140 P2 (count.py and MT wall clock): kept with numbers and an
ASan test of a constant escaping to a user callable, or dropped.

## Result: constants read in place; no static kind

Measured on master 5ce1786 (Release, `count.py`; MT wall clock with `--threads` on the
4-core cloud VM, three alternating runs each, which spread about 10%).

**How often the suite copies a constant.** A counting build (a mark on every object a
`ConstantExpression` owns, counted in `intrusive_ptr_add_ref`) found at most 18 counted
copies of a constant per render, against 2,000-6,000 counted copies overall: `chat_llama` 12,
`chat_mistral` 5, `html_autoescape` 4, `mitsuhiko_table_wide` 6, every other case 0. Where
they come from (gdb on the counting build):

| Site | Cases | What it is |
|---|---|---|
| `BinaryExpression::Evaluate` | `chat_llama`, `chat_mistral` | `'...' + m['role']`: a constant left operand was copied whenever the right one was not pure |
| `TupleCreator::Evaluate`, `ListAdapter::CreateAdapter` | `mitsuhiko_table_wide` | a list literal's long-string items, stored in a list built per render |
| `EvaluateCallParams`, `InvokeMacroRenderer` | `html_autoescape` | a long-string macro argument, stored in the macro's parameters |

**The static kind as proposed** (a flag on the object, `intrusive_ptr_add_ref`/`release`
skip flagged objects) costs a branch on every count operation: Render +0.3..+1.4% Ir on
every case (`mitsuhiko_table` +1.36%, `config_file` +0.87%, `macros` +0.75%), and gives
nothing measurable in MT on the suite, which scales close to linearly at 4 threads with or
without it (`chat_llama` 26-28k -> 97-108k renders/s). It only pays when a template copies
a constant in a hot loop: a 200-iteration loop over a list literal of long strings and a
long-string `~` ran 20k renders/s at 4 threads on master (1.5x one thread) and 57k with
the flag (4x). Not built: the Ir cost lands on every template, the gain only on that
pattern, and the escape the problem statement names would need guarding as well.

**Built: a constant left operand is read in place** (`BinaryExpression`, 0118's
`m_leftByRef`). A constant cannot be changed by evaluating the right operand, so the "right
operand is pure" condition never applied to it. This is the chat-template pattern: `'<|x|>'
+ message['role'] + ...` inside a loop over messages. Nothing is borrowed beyond the
expression, so there is no escape to guard. Counts vs 5ce1786: Render `chat_llama` -0.13%,
`chat_mistral` -0.09%, `chat_qwen` -0.11%, `html_autoescape` -0.18%, others +-0.00%; Load
+0.00..+0.35% (`expressions` +156 Ir: one kind check per binary operator). MT on a loop of
200 `"<|start_header_id|>" + m.role + "<|end_header_id|>\n\n" + m.content + "<|eot_id|>"`:
23-25k -> 40-43k renders/s at 4 threads (+75%; now 3.6x one thread, as other cases).

**Left as they are: stores of a constant** (list literal items, macro and call arguments).
They keep a counted copy, as any stored value must own what it holds. **Reopen if** a real
template shows a list literal or call argument of long-string constants in a hot loop under
MT; the flag design above is then the candidate, with the escape guard (a constant reaching
`CreateGenericList`/`CreateGenericMap` or a user callable turned into an owned copy) and an
ASan test for it.
