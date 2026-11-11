"""Template.generate yields Jinja2's items (docs/tasks/0436): one for each run of text, `{{ }}` value and macro or
call block output (RenderOptions::writeEachPiece). The render runs at the first item, on the caller's thread."""

import threading

import jinja2cpp
import pytest

LONG = "x" * 2000


def test_the_render_runs_at_the_first_item_on_the_callers_thread():
    threads = []
    tpl = jinja2cpp.Template("{{ f() }}" + LONG + "{{ f() }}")
    generator = tpl.generate(f=lambda: threads.append(threading.get_ident()) or "a")
    assert threads == []  # nothing runs before the first item, as a Python generator
    assert next(generator) == "a"
    assert threads == [threading.get_ident()] * 2
    assert "".join(generator) == LONG + "a"


def test_an_error_comes_after_the_items_before_it_and_ends_the_generator():
    def fail():
        raise ZeroDivisionError("boom")

    generator = jinja2cpp.Environment().from_string(LONG + "{{ fail() }}").generate(fail=fail)
    got = []
    with pytest.raises(ZeroDivisionError, match="boom"):
        for item in generator:
            got.append(item)
    assert "".join(got) == LONG
    assert list(generator) == []


@pytest.mark.parametrize("exception", [KeyboardInterrupt, SystemExit])
def test_a_base_exception_comes_after_the_items_before_it(exception):
    def fail():
        raise exception()

    got = []
    with pytest.raises(exception):
        for item in jinja2cpp.Environment().from_string("a{{ x }}b{{ fail() }}c").generate(x=1, fail=fail):
            got.append(item)
    assert got == ["a", "1", "b"]


def test_stream_buffers_the_items():
    source = "{% for i in range(300) %}{{ i }}-{% endfor %}"
    stream = jinja2cpp.Template(source).stream()
    stream.enable_buffering(size=3)
    items = list(stream)
    assert items[0] == "0-1"
    assert "".join(items) == jinja2cpp.Template(source).render()


@pytest.mark.parametrize(
    "source",
    [
        "<ul>{% for item in seq %}<li>{{ loop.index }} - {{ item }}</li>{%- endfor %}</ul>",
        "a{{ x }}b{% if x %}c{{ x }}{% endif %}d",
        "{% macro m(a) %}[{{ a }}]{% endmacro %}x{{ m(x) }}y{{ m(x)|upper }}",
        "{% macro m() %}[{{ caller() }}]{% endmacro %}x{% call m() %}{{ x }}{% endcall %}y",
        "{% set s %}a{{ x }}b{% endset %}{{ s }}|{% filter upper %}a{{ x }}{% endfilter %}",
        "{% block b %}B{{ x }}{% endblock %}E{% include 'inc' %}|",
    ],
)
def test_items_are_jinja2s(source):
    import jinja2

    items = []
    for module in (jinja2, jinja2cpp):
        env = module.Environment(loader=module.DictLoader({"inc": "I{{ x }}J"}))
        items.append(list(env.from_string(source).generate(seq=[0, 1], x=5)))
    assert items[1] == items[0]
