---
status: open
priority: low
area: api
depends: [0360]
touches: [include/jinja2cpp/chat_template.h, src/chat_template.cpp]
shares: [src/template_parser.cpp#ParseGeneration, src/value_methods.cpp#UnsafeMethod, include/jinja2cpp/error_info.h]
---
# Chat templates: the assistant token mask and the sandbox's SecurityError

**Problem.** The chat-template preset (0360, `ConfigureForChatTemplates`) renders what
transformers' `apply_chat_template` renders, byte for byte on test/chat_templates, but two
things transformers offers are not there:

- `{% generation %}` only renders its body. transformers also records where each body's text
  lands in the output (`return_assistant_tokens_mask=True`), which training pipelines use to
  mask the assistant turns. A host has no way to get those offsets.
- A call of a method the immutable sandbox forbids (`messages.append(x)`) fails with
  `ErrorCode::UndefinedError` and the sandbox's message (`access to attribute 'append' of
  'list' object is unsafe.`); transformers raises `SecurityError`. A host that wants to tell a
  rejected template from a template bug cannot do it by the code.

**Proposal.** Decide with the API owner whether the mask belongs in 2.0: a field of
`ChatTemplateOptions` or a render overload returning `(start, end)` offsets per generation
block, in characters of the output (narrow: bytes; transformers counts str characters, so the
narrow offsets need converting). For the error, either a new `ErrorCode::SecurityError`
(ABI-visible, record in docs/api-2.0.md) or a documented `extraParams` marker.

**Done when.** The decision is recorded; if taken, tests render a template with generation
blocks and compare the offsets with transformers' `generation_indices` for the
test/chat_templates conversations.
