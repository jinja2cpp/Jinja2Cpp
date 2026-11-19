"""Template.generate yields Jinja2's items (docs/tasks/0436): one for each run of text, `{{ }}` value and macro or
call block output (RenderOptions::writeEachPiece). The template runs as far as the items asked for, on the caller's
thread (docs/tasks/0457): the render runs on a fiber and switches back at each item."""

import contextvars
import gc
import sqlite3
import subprocess
import sys
import textwrap
import threading
import weakref

import jinja2
import jinja2cpp
import pytest
from jinja2cpp.sandbox import SandboxedEnvironment

LONG = "x" * 2000

lazy_only = pytest.mark.skipif(not jinja2cpp._jinja2cpp._lazy_generate_available(), reason="generate() renders eagerly here")


@pytest.fixture(params=["lazy", "eager"])
def mode(request):
    """The tests that hold in both modes run with the lazy render and with the eager fallback."""
    if request.param == "lazy" and not jinja2cpp._jinja2cpp._lazy_generate_available():
        pytest.skip("generate() renders eagerly here")
    jinja2cpp._jinja2cpp._set_lazy_generate(request.param == "lazy")
    yield request.param
    jinja2cpp._jinja2cpp._set_lazy_generate(True)


def test_the_render_runs_on_the_callers_thread(mode):
    threads = []
    tpl = jinja2cpp.Template("{{ f() }}" + LONG + "{{ f() }}")
    generator = tpl.generate(f=lambda: threads.append(threading.get_ident()) or "a")
    assert threads == []  # nothing runs before the first item, as a Python generator
    assert next(generator) == "a"
    assert threads[0] == threading.get_ident()
    assert "".join(generator) == LONG + "a"
    assert threads == [threading.get_ident()] * 2


def test_an_error_comes_after_the_items_before_it_and_ends_the_generator(mode):
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
def test_a_base_exception_comes_after_the_items_before_it(exception, mode):
    def fail():
        raise exception()

    got = []
    with pytest.raises(exception):
        for item in jinja2cpp.Environment().from_string("a{{ x }}b{{ fail() }}c").generate(x=1, fail=fail):
            got.append(item)
    assert got == ["a", "1", "b"]


def test_stream_buffers_the_items(mode):
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
        # docs/tasks/0456: folded constants, empty items, and calls whose value is one item however long
        "a{{ 1 }}b{{ none }}{{ true }}{{ 1.5 }}c{# d #}e{% raw %}r{% endraw %}",
        "a{{ y }}b{{ '' }}{% if 1 %}{% endif %}{{ x if not x }}",
        "{% macro m() %}{% endmacro %}[{{ m() }}]{% filter upper %}{% endfilter %}{% macro c() %}{{ caller() }}{% endmacro %}{% call c() %}{% endcall %}",
        "{% for i in [[1, [2]], 3] recursive %}({{ i if i is number else loop(i) }}){% endfor %}|{% for i in seq recursive %}{{ i }}{% endfor %}",
        "{% block b %}A{{ 5 }}B{% endblock %}{{ self.b() }}",
        "{% extends 'base' %}{% block b %}<{{ super() }}>{% endblock %}",
        "{% macro m() %}{% for i in range(300) %}ab{{ i }}{% endfor %}{% endmacro %}{{ m() }}{{ m()|length }}",
        "x" * 2000 + "{% if 1 %}" + "y" * 200 + "{{ 1 }}" + "z" * 200 + "{% endif %}{% set s = 'q' * 900 %}a{{ s }}b",
        "{% for i in [] recursive %}{{ i }}{% endfor %}{% for i in seq recursive %}{% endfor %}x",
        "{% autoescape y %}a{{ 1 }}b{{ 2 }}{{ 3 }}{% endautoescape %}{% autoescape true %}{{ '<' }}a{{ '<' }}{% endautoescape %}",
    ],
)
def test_items_are_jinja2s(source, mode):
    items = []
    for module in (jinja2, jinja2cpp):
        env = module.Environment(loader=module.DictLoader({"inc": "I{{ x }}J", "base": "A{% block b %}P{{ x }}Q{% endblock %}Z"}))
        items.append(list(env.from_string(source).generate(seq=[0, 1], x=5)))
    assert items[1] == items[0]


@pytest.mark.parametrize("autoescape", [False, True], ids=["plain", "autoescape"])
@pytest.mark.parametrize(
    "source",
    [
        "a{{ x }}b{{ 'c' }}d{{ x|safe }}",
        "{% autoescape y %}a{{ x }}b{{ 'k' }}c{% endautoescape %}{% autoescape true %}a{{ x }}{% endautoescape %}",
        "{% autoescape false %}a{{ x }}{% endautoescape %}",
        "{% macro m() %}<{{ x }}{{ caller() if caller }}{% endmacro %}{{ m() }}{% call m() %}c{% endcall %}",
        "{% filter upper %}a{{ x }}{% endfilter %}{% filter e %}<{% endfilter %}{% set s %}a{{ x }}{% endset %}{{ s }}",
        "{% for i in [1] %}{{ i }}{% endfor %}{% for i in [1] recursive %}{{ i }}{% endfor %}",
        "{% block b %}B{{ x }}{% endblock %}{{ self.b() }}{% include 'inc' %}{% if 1 %}t{% endif %}{{ y }}",
    ],
)
def test_items_are_markup_where_jinja2s_are(source, autoescape, mode):
    # docs/tasks/0493: under autoescape Jinja2 yields escape(value), Markup, for a value it evaluates at run time
    items = []
    for module in (jinja2, jinja2cpp):
        env = module.Environment(autoescape=autoescape, loader=module.DictLoader({"inc": "I{{ x }}J"}))
        items.append([(type(item).__name__, str(item)) for item in env.from_string(source).generate(x="<", y=True)])
    assert items[1] == items[0]


# What the template does and what the consumer gets, in order: the same as Jinja2's for each template
ON_DEMAND = [
    "a{{ f(1) }}b{{ f(2) }}",
    "{% macro m() %}{{ f(1) }}-{{ f(2) }}{% endmacro %}x{{ m() }}y{{ f(3) }}",
    "{% for x in gen() %}[{{ x }}]{% endfor %}{{ f(9) }}",
    "a{% include 'inc' %}b{{ f(3) }}",
    "{% set s %}{{ f(1) }}{% endset %}{{ f(2) }}{{ s }}",
    "{% filter upper %}{{ f(1) }}x{% endfilter %}{{ f(2) }}",
    "{% extends 'base' %}{% block b %}{{ f(1) }}{% endblock %}",
    "{% for i in range(3) %}{% if i %}{{ f(i) }}{% endif %}{{ i }}{% endfor %}",
    "{% macro m() %}<{{ caller() }}>{% endmacro %}{% call m() %}{{ f(1) }}{% endcall %}{{ f(2) }}",
    # A recursive loop is one item in Jinja2 (task 0456), so its calls come before it
    "{% for i in [1, 2] recursive %}{{ f(i) }}{% endfor %}{{ f(3) }}",
    "{% block b %}{{ f(1) }}{% endblock %}{{ self.b() }}{{ f(2) }}",
]
TEMPLATES = {"inc": "I{{ f(1) }}J{{ f(2) }}", "base": "<{% block b %}{% endblock %}>{{ f(2) }}"}


def _events(module, source):
    log = []

    def f(n):
        log.append(("call", n))
        return n

    def gen():
        for i in range(2):
            log.append(("produce", i))
            yield i

    env = module.Environment(loader=module.DictLoader(TEMPLATES))
    for item in env.from_string(source).generate(f=f, gen=gen):
        log.append(("item", item))
    return log


@lazy_only
@pytest.mark.parametrize("source", ON_DEMAND)
def test_on_demand_matches_jinja2(source):
    assert _events(jinja2cpp, source) == _events(jinja2, source)


@lazy_only
def test_a_later_part_does_not_run_before_its_item_is_asked_for():
    calls = []
    generator = jinja2cpp.Template("{{ f(1) }}" + LONG + "{{ f(2) }}").generate(f=lambda n: calls.append(n) or n)
    assert next(generator) == "1"
    assert calls == [1]
    assert next(generator) == LONG
    assert calls == [1]
    generator.close()
    assert calls == [1]


@lazy_only
def test_callables_use_the_callers_thread_state():
    local = threading.local()
    local.value = "mine"
    connection = sqlite3.connect(":memory:")  # refuses use from another thread
    lock = threading.RLock()
    lock.acquire()
    try:

        def query():
            # An RLock the caller holds is re-entered only on its own thread
            with lock:
                return connection.execute("select 40 + 2").fetchone()[0]

        generator = jinja2cpp.Template("{{ local.value }}-{{ query() }}").generate(local=local, query=query)
        assert list(generator) == ["mine", "-", "42"]
    finally:
        lock.release()
        connection.close()


@lazy_only
def test_context_variables_of_the_resumer():
    var = contextvars.ContextVar("var", default="outer")
    generator = jinja2cpp.Template("{{ get() }}|{{ get() }}").generate(get=var.get)
    context = contextvars.copy_context()
    context.run(var.set, "inner")
    assert context.run(next, generator) == "inner"
    assert next(generator) == "|"
    assert next(generator) == "outer"


@lazy_only
def test_close_and_throw_stop_later_calls():
    calls = []
    tpl = jinja2cpp.Template("{% for i in range(5) %}{{ f(i) }}{% endfor %}")
    generator = tpl.generate(f=lambda n: calls.append(n) or n)
    assert next(generator) == "0"
    generator.close()
    assert calls == [0]
    assert list(generator) == []

    generator = tpl.generate(f=lambda n: calls.append(n) or n)
    calls.clear()
    assert next(generator) == "0"
    with pytest.raises(LookupError):
        generator.throw(LookupError("stop"))
    assert calls == [0]


@lazy_only
def test_an_abandoned_generator_lets_go_of_its_params():
    class Holder:
        x = 1

    holder = Holder()
    collected = []
    ref = weakref.ref(holder, lambda _: collected.append(True))
    generator = jinja2cpp.Template("{{ h.x }}{% for i in range(3) %}{{ i }}{% endfor %}").generate(h=holder)
    assert next(generator) == "1"
    del holder
    del generator
    gc.collect()
    assert collected == [True]
    assert ref() is None


@lazy_only
@pytest.mark.parametrize("how", ["keyword", "positional", "global", "closure"])
def test_a_cycle_through_a_suspended_generator_is_collected(how):
    # docs/tasks/0462: the generator is stored in a value its own render holds
    class Holder:
        x = 1

    holder = Holder()
    collected = []
    ref = weakref.ref(holder, lambda _: collected.append(True))
    source = "{{ h.x }}{% for i in range(3) %}{{ i }}{% endfor %}"
    if how == "global":
        env = jinja2cpp.Environment()
        env.globals["h"] = holder
        generator = env.from_string(source).generate()
    elif how == "closure":
        generator = jinja2cpp.Template(source).generate(h=holder, f=lambda: generator)
    elif how == "positional":
        generator = jinja2cpp.Template(source).generate({"h": holder})
    else:
        generator = jinja2cpp.Template(source).generate(h=holder)
    holder.generator = generator
    assert next(generator) == "1"
    assert next(generator) == "0"
    del holder, generator
    if how == "global":
        del env
    gc.collect()
    assert collected == [True]
    assert ref() is None


@lazy_only
def test_a_cycle_collected_on_another_thread_does_not_resume_there():
    class Holder:
        x = 1

    def make():
        holder = Holder()
        holder.generator = jinja2cpp.Template("{{ h.x }}{{ h.x }}").generate(h=holder)
        assert next(holder.generator) == "1"
        return weakref.ref(holder)

    ref = make()
    thread = threading.Thread(target=gc.collect)
    thread.start()
    thread.join()
    gc.collect()
    # Freed without running on the other thread: what its frames held stays (docs/python.md)
    assert ref() is None or ref().x == 1


@lazy_only
def test_two_generators_interleave_sandboxed_and_plain():
    from jinja2cpp.exceptions import SecurityError

    class Secret:
        def _private(self):
            return "leak"

    plain = jinja2cpp.Template("{% for i in range(3) %}{{ i }}{% endfor %}{{ s.__class__.__name__ }}").generate(s=Secret())
    sandboxed = SandboxedEnvironment().from_string("{% for i in range(3) %}{{ i }}{% endfor %}{{ s._private() }}").generate(s=Secret())
    got_plain = []
    got_sandboxed = []
    error = None
    for _ in range(4):
        got_plain.append(next(plain))
        try:
            got_sandboxed.append(next(sandboxed))
        except SecurityError as raised:
            error = raised
    assert got_plain == ["0", "1", "2", "Secret"]
    assert got_sandboxed == ["0", "1", "2"]
    assert isinstance(error, SecurityError)


@lazy_only
def test_next_from_its_own_callable_is_refused():
    def again():
        return next(me)

    me = jinja2cpp.Template("a{{ again() }}").generate(again=again)
    assert next(me) == "a"
    with pytest.raises(ValueError, match="already executing"):
        next(me)


@lazy_only
def test_nested_generate_gives_each_render_its_items():
    inner = jinja2cpp.Template("{% for i in range(3) %}<{{ i }}>{% endfor %}")
    state = {}

    def start():
        state["gen"] = inner.generate()
        return next(state["gen"])

    def step():
        return next(state["gen"])

    outer = jinja2cpp.Template("[{{ start() }}]{{ step() }}{{ step() }}|").generate(start=start, step=step)
    assert list(outer) == ["[", "<", "]", "0", ">", "|"]
    assert list(state["gen"]) == ["<", "1", ">", "<", "2", ">"]


@lazy_only
def test_next_from_another_thread_is_refused():
    generator = jinja2cpp.Template("{% for i in range(3) %}{{ i }}{% endfor %}").generate()
    assert next(generator) == "0"
    errors = []

    def other():
        try:
            next(generator)
        except RuntimeError as error:
            errors.append(str(error))

    thread = threading.Thread(target=other)
    thread.start()
    thread.join()
    assert errors and "render()" in errors[0]
    # The error ends the generator, as any exception out of a generator does; its render is abandoned there
    assert list(generator) == []


@lazy_only
def test_deep_recursion_is_recursion_error():
    env = jinja2cpp.Environment()
    with pytest.raises(RecursionError):
        list(env.from_string("{% macro r(n) %}{{ r(n + 1) }}{% endmacro %}{{ r(0) }}").generate())

    deep = []
    for _ in range(200000):
        deep = [deep]

    with pytest.raises(RecursionError):
        list(env.from_string("{{ f() }}").generate(f=lambda: repr(deep)))
    assert list(env.from_string("{{ 1 }}").generate()) == ["1"]


@lazy_only
@pytest.mark.parametrize("where", ["main", "daemon"])
def test_the_interpreter_exits_with_a_suspended_generator(where):
    script = textwrap.dedent(
        f"""
        import threading, jinja2cpp
        tpl = jinja2cpp.Template("{{% for i in range(5) %}}{{{{ i }}}}{{% endfor %}}")
        keep = []
        def run():
            g = tpl.generate()
            next(g)
            keep.append(g)
            threading.Event().wait() if {where == "daemon"!r} else None
        if {where == "daemon"!r}:
            threading.Thread(target=run, daemon=True).start()
            import time; time.sleep(0.2)
        else:
            run()
        """
    )
    result = subprocess.run([sys.executable, "-c", script], capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stderr


# --- generate_async: the lazy render as an async iterator (docs/tasks/0461) -----------------------------------------


def _async_items(template, **params):
    import asyncio

    async def collect():
        return [item async for item in template.generate_async(**params)]

    return asyncio.run(collect())


@pytest.mark.parametrize(
    "source",
    [
        "a{{ x }}b{% for i in range(3) %}<{{ i }}>{% endfor %}",
        "{% macro m() %}M{{ caller() }}{% endmacro %}{% call m() %}C{% endcall %}",
        "",
    ],
)
def test_generate_async_yields_the_items_of_jinja2s(source, mode):
    native = _async_items(jinja2cpp.Environment().from_string(source), x=1)
    oracle = _async_items(jinja2.Environment(enable_async=True).from_string(source), x=1)
    assert native == oracle
    assert native == list(jinja2cpp.Environment().from_string(source).generate(x=1))


@lazy_only
def test_generate_async_renders_each_item_when_it_is_asked_for():
    import asyncio

    calls = []
    template = jinja2cpp.Template("{{ f(1) }}" + LONG + "{{ f(2) }}")

    async def run():
        items = template.generate_async(f=lambda n: calls.append((n, threading.get_ident())) or str(n))
        assert calls == []
        assert await items.__anext__() == "1"
        assert [n for n, _ in calls] == [1]  # the second call has not run yet
        rest = [item async for item in items]
        assert rest == [LONG, "2"]
        return threading.get_ident()

    loop_thread = asyncio.run(run())
    assert [thread for _, thread in calls] == [loop_thread, loop_thread]


@lazy_only
def test_generate_async_closed_early_stops_the_render():
    import asyncio

    calls = []
    template = jinja2cpp.Template("{{ f(1) }}{{ f(2) }}{{ f(3) }}")

    async def run():
        items = template.generate_async(f=lambda n: calls.append(n) or str(n))
        assert await items.__anext__() == "1"
        await items.aclose()

    asyncio.run(run())
    assert calls == [1]


def test_generate_async_raises_after_the_items_before_the_error(mode):
    import asyncio

    template = jinja2cpp.Environment(undefined=jinja2cpp.StrictUndefined).from_string("a{{ b }}{{ missing }}")

    async def run():
        seen = []
        with pytest.raises(jinja2cpp.UndefinedError):
            async for item in template.generate_async(b="B"):
                seen.append(item)
        return seen

    assert asyncio.run(run()) == ["a", "B"]


def test_generate_async_of_an_async_environment_is_jinja2s():
    # enable_async=True makes the environment Jinja2's; its generate_async awaits what callables return
    import asyncio

    async def value():
        return "V"

    template = jinja2cpp.Environment(enable_async=True).from_string("{{ f() }}!")
    assert "".join(_async_items(template, f=value)) == "V!"
    assert type(template).generate_async is not jinja2cpp.Template.generate_async
    assert asyncio.iscoroutinefunction(value)


@lazy_only
def test_starlette_streams_generate_async():
    # Starlette's StreamingResponse iterates an async iterator on the event loop's thread, where it runs a sync
    # iterator's next() in a thread pool (iterate_in_threadpool), which a lazy generate() may refuse (docs/python.md).
    # The app is called as an ASGI server calls it, so that each chunk is seen when it is sent
    import asyncio

    pytest.importorskip("starlette")
    from starlette.applications import Starlette
    from starlette.responses import StreamingResponse
    from starlette.routing import Route

    source = "{% for i in range(20) %}<{{ f(i) }}>" + "y" * 100 + "{% endfor %}"
    events = []

    def f(i):
        events.append(("call", i))
        return str(i)

    template = jinja2cpp.Template(source)

    async def endpoint(request):
        return StreamingResponse(template.generate_async(f=f), media_type="text/plain")

    app = Starlette(routes=[Route("/", endpoint)])

    async def get(path):
        chunks = []

        async def receive():
            await asyncio.sleep(3600)  # the client never disconnects
            return {"type": "http.disconnect"}

        async def send(message):
            if message["type"] == "http.response.body" and message.get("body"):
                events.append(("chunk", len(chunks)))
                chunks.append(message["body"].decode())

        scope = {"type": "http", "asgi": {"version": "3.0"}, "http_version": "1.1", "method": "GET", "scheme": "http",
                 "path": path, "raw_path": path.encode(), "query_string": b"", "root_path": "", "headers": [],
                 "server": ("test", 80), "client": ("test", 1)}
        await app(scope, receive, send)
        return "".join(chunks)

    body = asyncio.run(get("/"))
    assert body == jinja2.Template(source).render(f=str)
    # The first chunk is sent before the second callable runs
    assert events.index(("chunk", 0)) < events.index(("call", 1))
