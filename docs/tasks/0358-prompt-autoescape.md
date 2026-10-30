---
status: open
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

**Done when.** Table rows in `test/filters_test.cpp` (narrow and wide) show that
untrusted text containing the configured specials, tags and backtick runs cannot close
a fence or emit a special token, both through the filters and through the environment
mode. `|safe` and prompt-safe values pass through unchanged. The filters are specified
in `docs/cpp-only-filters.md`. A fuzz target or a differential property test checks that
no output of the mode contains a configured special token.

**Next.** A per-model registry of special tokens (from a tokenizer's
`special_tokens_map`) may belong to the host, not the library; decide in the plan.
