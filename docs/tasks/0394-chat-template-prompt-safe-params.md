---
status: open
priority: low
area: api
depends: [0358]
touches: [src/chat_template.cpp, include/jinja2cpp/chat_template.h]
---
# Name the prompt-safe chat template params and read specials from a tokenizer config

Filed by the 0358/0359 plan (Ruslan 2026-11-05).

**Problem.** In prompt mode a chat template's host params that carry special tokens on purpose
(`bos_token`, `eos_token`) must be passed as `jinja2::Markup`, and the host has to build the
specials list from its tokenizer itself.

**Proposal.** A `ChatTemplateOptions` field naming the params that are prompt-safe, and an example
(not library code) that reads the added tokens marked `special` from `tokenizer_config.json` into
a `PromptEscaper`.

**Done when.** A chat template test renders in prompt mode with `bos_token` as a plain string named
prompt-safe, and the example builds.
