# Hugging Face chat templates

Real `chat_template`s of widely used models, rendered by Jinja2C++ and by transformers'
`apply_chat_template` environment, compared byte for byte in narrow and wide mode
(docs/tasks/0360). Inference servers render these per request, so this is the workload
where Jinja2C++ has to match Python exactly.

| Path | What |
|---|---|
| `templates/*.jinja` | The templates, copied from llama.cpp's `models/templates/` (fetched there from the Hub) |
| `SOURCES.json` | Each template's Hugging Face repo, model license and sha256, and the llama.cpp commit |
| `conversations.py` | The chats rendered through every template, and a few for one template only |
| `generate.py` | Renders them with pinned transformers into `expected/`; `--check` fails if stale |
| `expected/` | Generated: the contexts (`conversations.json`) and each template's results |
| `divergences.txt` | Allow-list of renders that differ, with kind, owning task and reason |
| `chat_template_test.cpp` | The gtest suite (`ctest -R chat_templates`) |
| `update_templates.py` | Refreshes `templates/` from a llama.cpp checkout |
| `requirements.txt` | Pinned transformers; Jinja2 and MarkupSafe come from `test/parity/requirements.txt` |

## The oracle

transformers 5.22.1 compiles a chat template in
`chat_template_utils._compile_jinja_template`: an `ImmutableSandboxedEnvironment` with
`trim_blocks`, `lstrip_blocks`, `jinja2.ext.loopcontrols` and the `{% generation %}` tag, a
`tojson` filter of its own (`json.dumps` with `ensure_ascii=False`, keys in insertion order, no
HTML escaping), and the globals `raise_exception(msg)` and `strftime_now(fmt)`. `generate.py`
calls that function, so the expectations follow transformers' code rather than a copy of it.
`strftime_now` reads a fixed clock (2025-03-14 09:26:53); `bos_token` and `eos_token` are
placeholders, so no tokenizer is needed. The context stored in `expected/conversations.json`
is the one the template sees, after transformers adds the reasoning-field aliases.

Run under the Python of `.python-version` (3.12), as the parity corpus:

```bash
uv venv -p python3.12 /tmp/py312 && uv pip install -p /tmp/py312/bin/python -r test/parity/requirements.txt -r test/chat_templates/requirements.txt
/tmp/py312/bin/python test/chat_templates/generate.py
```

## What the suite checks

Each template is rendered with each conversation. Where transformers renders, Jinja2C++ must
render the same text; where transformers fails, Jinja2C++ must fail too, and a
`raise_exception` message must appear in the Jinja2C++ error. The suite configures the
environment with `ConfigureForChatTemplates` (`jinja2cpp/chat_template.h`, docs/tasks/0360) and
runs only in builds with `JINJA2CPP_WITH_CHAT_TEMPLATES=ON`. `divergences.txt` lists the renders
that differ, none at present. A listed render that matches fails the suite, so the list stays
exact. To rebuild it:

```bash
JINJA2CPP_CHAT_TEMPLATES_RESULTS=/tmp/chat.txt build/jinja2cpp_tests --gtest_filter='ChatTemplate*'
```

writes `<id> <kind>` for every render that differs (`wide.<id>` for the wide run).

## Licenses

The templates are part of the model repositories and keep their licenses (Apache-2.0, MIT,
the Llama 3.x community licenses, Gemma terms, CC-BY-NC-4.0 for Command-R and others); llama.cpp
redistributes the same files. `SOURCES.json` records the repo and license where they were
checked; `null` means not yet checked against the model card (the Hub was not reachable when the
collection was made). They are test data only and are not installed or linked into the library.
