"""Template.generate renders on demand (docs/tasks/0436): a worker thread runs the render and takes turns with the
consumer, at most one batch of output ahead of it."""

import contextvars
import gc
import os
import signal
import subprocess
import sys
import threading

import jinja2cpp
import pytest

LONG = "x" * 2000


def marking_template(source):
    seen = []

    def mark(value):
        seen.append(value)
        return value

    return jinja2cpp.Environment().from_string(source), seen, mark


def test_next_returns_before_a_later_part_runs():
    tpl, seen, mark = marking_template("{{ mark(1) }}" + LONG + "{{ mark(2) }}")
    generator = tpl.generate(mark=mark)
    assert seen == []  # nothing runs before the first item, as a Python generator
    first = next(generator)
    assert first.startswith("1")
    assert seen == [1]
    assert first + "".join(generator) == tpl.render(mark=lambda v: v)
    assert seen == [1, 2]


def test_close_ends_the_render():
    tpl, seen, mark = marking_template("{{ mark(1) }}" + LONG + "{{ mark(2) }}")
    generator = tpl.generate(mark=mark)
    next(generator)
    generator.close()
    assert seen == [1]
    assert list(generator) == []


def test_a_dropped_generator_ends_the_render():
    tpl, seen, mark = marking_template("{{ mark(1) }}" + LONG + "{{ mark(2) }}")
    generator = tpl.generate(mark=mark)
    next(generator)
    del generator
    gc.collect()
    assert seen == [1]


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


def test_the_render_sees_the_consumers_context():
    var = contextvars.ContextVar("var", default="unset")
    token = var.set("consumer")
    try:
        assert "".join(jinja2cpp.Template("{{ f() }}").generate(f=var.get)) == "consumer"
    finally:
        var.reset(token)


def test_next_inside_the_render_is_refused():
    holder = {}

    def again():
        next(holder["generator"])

    holder["generator"] = jinja2cpp.Template("{{ again() }}").generate(again=again)
    with pytest.raises(ValueError, match="already executing"):
        next(holder["generator"])


def test_generate_inside_a_render_and_on_another_thread():
    inner = jinja2cpp.Template("{% for i in range(3) %}<{{ i }}>{% endfor %}" + LONG)
    outer = jinja2cpp.Template("{{ f() }}|{{ f() }}")
    expected = outer.render(f=inner.render)
    assert "".join(outer.generate(f=lambda: "".join(inner.generate()))) == expected
    results = []
    thread = threading.Thread(target=lambda: results.append("".join(outer.generate(f=inner.render))))
    thread.start()
    thread.join()
    assert results == [expected]


def test_stream_buffers_the_items():
    stream = jinja2cpp.Template("{% for i in range(300) %}{{ i }}-{% endfor %}").stream()
    stream.enable_buffering(size=2)
    assert "".join(stream) == jinja2cpp.Template("{% for i in range(300) %}{{ i }}-{% endfor %}").render()


@pytest.mark.skipif(not hasattr(signal, "SIGINT") or os.name == "nt", reason="needs os.kill with SIGINT")
@pytest.mark.parametrize("source", ["{% for i in range(100000000) %}{% if i < 0 %}x{% endif %}{% endfor %}", "{{ spin() }}"])
def test_ctrl_c_stops_a_render_that_writes_nothing(source):
    # The consumer waits for the worker on the main thread; Ctrl-C raises KeyboardInterrupt there and ends the render
    stop = threading.Event()

    def spin():
        while not stop.is_set():
            pass

    timer = threading.Timer(0.2, lambda: os.kill(os.getpid(), signal.SIGINT))
    timer.start()
    try:
        generator = jinja2cpp.Template(source).generate(spin=spin)
        with pytest.raises(KeyboardInterrupt):
            next(generator)
        assert list(generator) == []
    finally:
        stop.set()
        timer.cancel()


def test_an_unfinished_generator_does_not_hold_up_the_exit():
    code = "import jinja2cpp\ng = jinja2cpp.Template('{{ 1 }}' + 'x' * 2000 + '{{ 2 }}').generate()\nprint(next(g)[:1])\n"
    result = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True, timeout=60, env=os.environ)
    assert result.returncode == 0, result.stderr
    assert result.stdout == "1\n"
    assert "leaked" not in result.stderr


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
