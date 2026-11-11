"""Chat templates through the module, and transformers' patch() (docs/tasks/0355, 0248 plan D9).

``test/chat_templates`` holds real chat templates and what transformers renders from each
conversation (docs/tasks/0360). Through ``chat_template_environment()``, configured as
transformers' ``_compile_jinja_template`` configures Jinja2, every render must be the same:
the same text, or a failure where transformers fails, with ``raise_exception``'s message.
"""

import datetime
import json
import pathlib

import jinja2cpp
import pytest
from jinja2cpp.integrations import transformers as integration

CHAT = pathlib.Path(__file__).resolve().parents[2] / "test" / "chat_templates"
# generate.py's clock: 2025-03-14 09:26:53 local time
FIXED_NOW = datetime.datetime(2025, 3, 14, 9, 26, 53).timestamp()


def _corpus():
    conversations = json.loads((CHAT / "expected" / "conversations.json").read_text(encoding="utf-8"))
    for path in sorted((CHAT / "expected").glob("*.json")):
        if path.name == "conversations.json":
            continue
        data = json.loads(path.read_text(encoding="utf-8"))
        yield path.stem, (CHAT / "templates" / data["template"]).read_text(encoding="utf-8"), data["renders"], conversations


def _listed():
    """test/chat_templates/divergences.txt: the renders where the C++ suite differs too."""
    listed = set()
    for raw in (CHAT / "divergences.txt").read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if line and not line.startswith("#") and not line.startswith("wide."):
            listed.add(line.split()[0])
    return listed


def test_chat_templates_render_as_transformers():
    env = integration.chat_template_environment(lambda: FIXED_NOW)
    listed = _listed()
    renders, problems = 0, []
    for name, source, expected, conversations in _corpus():
        template = env.from_string(source)
        for conversation, want in expected.items():
            renders += 1
            case = f"{name}.{conversation}"
            try:
                got = {"output": template.render(**conversations[conversation])}
            # TypeError and ValueError where Jinja2 raises them (0432)
            except (jinja2cpp.TemplateError, TypeError, ValueError) as error:
                got = {"error": type(error).__name__, "message": str(error)}
            if "output" in want:
                same = got.get("output") == want["output"]
            else:
                # raise_exception's message must reach the error; another failure just has to fail
                same = "error" in got and (want["error"] != "TemplateError" or want["message"] in got["message"])
            if same == (case in listed):
                problems.append(f"{case}: {'matches but is listed' if same else 'differs'}: {str(got)[:200]}")
    print(f"\nchat templates through the module: {renders} renders, {len(problems)} problems")
    assert renders > 700
    assert not problems, "\n".join(problems)


def test_chat_environment_is_transformers_one():
    env = integration.chat_template_environment(lambda: FIXED_NOW)
    assert isinstance(env, jinja2cpp.sandbox.ImmutableSandboxedEnvironment)
    tpl = env.from_string(
        "{% for m in messages %}{{ m|tojson }}\n  {% if loop.index > 1 %}{% break %}{% endif %}{% endfor %}"
        "{{ strftime_now('%Y-%m-%d %H:%M') }}|{% generation %}g{% endgeneration %}"
    )
    out = tpl.render(messages=[{"b": "<é>", "a": 1}, {"c": None}, {"d": 2}])
    assert out == '{"b": "<é>", "a": 1}\n{"c": null}\n2025-03-14 09:26|g'
    with pytest.raises(jinja2cpp.TemplateError, match="stop here"):
        env.from_string("{{ raise_exception('stop here') }}").render()
    # ImmutableSandboxedEnvironment: the template cannot change the data
    with pytest.raises(jinja2cpp.SecurityError):
        env.from_string("{% set l = [] %}{{ l.append(1) }}").render()


@pytest.fixture
def transformers_utils():
    # a fixture, not a module-level importorskip, so the tests above run without transformers
    return pytest.importorskip("transformers.utils.chat_template_utils", reason="transformers not installed")


@pytest.fixture
def patched(transformers_utils):
    integration.patch()
    try:
        yield transformers_utils
    finally:
        integration.unpatch()


def _render(utils, source, conversation, **kwargs):
    context = dict(conversation)
    messages = context.pop("messages")
    for key in ("tools", "documents", "add_generation_prompt"):
        kwargs.setdefault(key, context.pop(key, None))
    return utils.render_jinja_template(conversations=[messages], chat_template=source, **context, **kwargs)


def test_patch_installs_and_restores(transformers_utils):
    original = transformers_utils._compile_jinja_template
    integration.patch()
    integration.patch()  # idempotent
    try:
        assert integration.is_patched()
        assert isinstance(transformers_utils._compile_jinja_template("{{ 1 }}"), integration.ChatTemplate)
    finally:
        integration.unpatch()
    assert transformers_utils._compile_jinja_template is original
    assert not integration.is_patched()


def test_patch_renders_as_transformers(patched):
    # Templates that do not read the clock render the same text through transformers' own entry point
    checked = 0
    for name, source, expected, conversations in _corpus():
        if "strftime_now" in source:
            continue
        for conversation, want in expected.items():
            if "output" not in want:
                continue
            rendered, _ = _render(patched, source, conversations[conversation])
            assert rendered == [want["output"]], f"{name}.{conversation}"
            checked += 1
    assert checked > 300


def test_patch_assistant_mask_uses_transformers(patched):
    source = "{% for m in messages %}{% if m.role == 'assistant' %}{% generation %}{{ m.content }}{% endgeneration %}{% else %}{{ m.content }}{% endif %}{% endfor %}"
    messages = [{"role": "user", "content": "hi "}, {"role": "assistant", "content": "hello"}]
    rendered, indices = patched.render_jinja_template(conversations=[messages], chat_template=source, return_assistant_tokens_mask=True)
    assert rendered == ["hi hello"]
    assert indices == [[(3, 8)]]
    # Without the mask the engine renders {% generation %} itself
    assert patched._compile_jinja_template(source)._tracked() is False
    rendered, _ = patched.render_jinja_template(conversations=[messages], chat_template=source)
    assert rendered == ["hi hello"]


def test_patch_hands_unloadable_templates_to_transformers(patched):
    # A syntax jinja2cpp lacks (a tuple of slices Jinja2 folds by a conditional, task 0463) compiles in
    # transformers' environment
    compiled = patched._compile_jinja_template('{{ 1 }}{{ (()[:, :] if 1 > 0) is undefined and "" }}{{ 2 }}')
    assert not isinstance(compiled, integration.ChatTemplate)
    assert compiled.render() == "12"
    # transformers reads the variables through Jinja2's AST: the proxy gives its environment
    assert patched._get_template_variables("{{ messages }}{{ foo }}") == frozenset({"messages", "foo"})
