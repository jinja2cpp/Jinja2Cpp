---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/784
priority: medium
area: api
touches: [src/prompt_escape.cpp, src/prompt_escape.h]
shares: [include/jinja2cpp/template_env.h, src/filter_factories.cpp, src/filters.h, test/filters_test.cpp, docs/cpp-only-filters.md, docs/api-2.0.md, CMakeLists.txt]
---
# Prompt autoescape: `fence`, `neutralize` and an escaping mode for LLM prompts

**Problem.** Templates that build LLM prompts put untrusted text (user messages, retrieved
documents, tool output) into the same channel as the instructions. That is the
injection problem HTML autoescape solved for markup, but a prompt has no fixed escape
syntax: what breaks out of a prompt is a delimiter the model trusts (`</document>`, a
closing code fence) or a tokenizer special token (`<|im_start|>`, `<|eot_id|>`,
`[INST]`). Today the escaping is ad-hoc string replacement in host code, outside the
template, so it is easy to forget on one variable and nothing marks a value as already
handled.

**Proposal.** Reuse the autoescape machinery (`Markup`, the `is escaped` test, `|safe`):
- `fence(lang="")`: wraps a value in a Markdown code fence whose backtick run is longer
  than any run inside the value, so the content cannot close it. The result is
  Markup-like (prompt-safe) so it is not escaped twice.
- `neutralize(specials=none, tags=none)`: breaks every occurrence of the given special
  tokens and of opening/closing tags with the given names. Insert a zero-width or visible
  separator; the exact output is the design question. The defaults come from a list set
  on the environment.
- An environment setting next to `autoescape` that selects a prompt escaper instead of
  HTML, applying `neutralize` with the environment's specials to every `{{ }}` that is
  not prompt-safe. Host code registers a model's special tokens once.
The settings field and its type are public API: get an architect plan and a recorded
decision in `docs/api-2.0.md` before implementing, and land it in the API wave before
the G1 freeze.

**Decision (Ruslan 2026-11-05, docs/api-2.0.md 3.3 and section 7 item 9).** All recommendations
of the architect plan were approved; the filters and the mode are specified in
docs/cpp-only-filters.md. Steps, one PR each, in order:
1. PR 0 (docs): the decision and the filter specification; follow-ups 0392-0396.
2. PR 1: `include/jinja2cpp/prompt.h` (`PromptEscapeOptions`, `PromptEscaper`), the
   `Settings::promptEscaper` field, `src/prompt_escape.{h,cpp}` (a trie over the patterns and one
   over the reversed patterns for the edge rule, a first-unit table so text without a first
   character is one scan), the `neutralize` and `fence` filters, a `PromptFilterTest` fixture
   (narrow and wide) with the specials `<|im_start|>`, `<|im_end|>`, `[INST]` and the tag
   `document`. Codex review and our own security review (CLAUDE.md).
3. PR 2: `Settings::autoescapeMode`, prompt mode (escaper chosen in the slow path of
   `WriteEscaped`/`MarkupEscape` through a non-virtual accessor on the renderer callback; no
   change to `WriteOutput`), string literals prompt-safe in prompt mode, the escape-site audit
   (`|e`, `forceescape`, `urlize`, `xmlattr`, `tojson`), the fuzz target
   `fuzz/fuzz_prompt_escape.cpp` (specials from private-use code points the template source never
   contains; property: no output holds a configured special) with a fixed-seed gtest sample,
   `bench/count.py` on `html_autoescape` and two cases without autoescape.

**Done when.** Table rows in `test/filters_test.cpp` (narrow and wide) show that
untrusted text containing the configured specials, tags and backtick runs cannot close
a fence or emit a special token, both through the filters and through the environment
mode. `|safe` and prompt-safe values pass through unchanged. The filters are specified
in `docs/cpp-only-filters.md`. A fuzz target or a differential property test checks that
no output of the mode contains a configured special token.

**Next.** A per-model registry of special tokens (from a tokenizer's
`special_tokens_map`) may belong to the host, not the library; decide in the plan.
