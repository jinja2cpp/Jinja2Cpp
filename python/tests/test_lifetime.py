"""References: no leaks across many renders, cycles collected, Ctrl-C stops a long render."""

import gc
import signal
import sys
import threading
import weakref

import jinja2cpp
import markupsafe
import pytest

LOOP = 300


class Obj:
    def __init__(self):
        self.items = [1, 2, 3]

    def method(self, x):
        return x


SOURCE = """{% macro m(v) %}{{ v }}{% endmacro %}
{% for k, v in d.items() %}{{ k }}{{ v }}{{ m(v) }}{% endfor %}
{{ o.method(o) }}{{ o.items|join }}{{ l[1:] }}{{ t }}{{ g|list }}{{ s|upper }}{{ mk }}
{{ f(d, o, l, 2**70, 'x'|safe, missing) }}{{ d|tojson }}{{ sd|dictsort }}{{ o.nope is defined }}
{{ 1|pyfilter }}{{ 1 is pytest }}{{ d|items|list }}{{ mk ~ 'a' }}"""


def _render(tpl, objects):
    d, o, l, t, s, mk = objects
    return tpl.render(d=d, sd={"b": 1, "a": d}, o=o, l=l, t=t, g=iter([1, 2]), s=s, mk=mk, f=lambda *a: len(a))


def _counts(objects):
    return [sys.getrefcount(x) for x in objects]


@pytest.mark.parametrize("autoescape", [False, True])
def test_no_reference_leaks(autoescape):
    env = jinja2cpp.Environment(autoescape=autoescape)
    env.filters["pyfilter"] = lambda v: v
    env.tests["pytest"] = lambda v: True
    tpl = env.from_string(SOURCE)
    objects = ({"a": 1, "2": "b"}, Obj(), [1, 2, 3], (1, "a"), "żółw", markupsafe.Markup("<i>"))
    # Warm up: caches of the interpreter and of the module fill on the first renders
    for _ in range(3 * LOOP):
        _render(tpl, objects)
    gc.collect()
    before = _counts(objects)
    blocks_before = sys.getallocatedblocks()
    for _ in range(10 * LOOP):
        _render(tpl, objects)
    gc.collect()
    assert _counts(objects) == before
    # A leak of one object per render would leave 10 * LOOP blocks or more behind; the
    # interpreter's free lists move the count by a few hundred either way
    assert sys.getallocatedblocks() - blocks_before < 3 * LOOP


def test_errors_do_not_leak():
    obj = Obj()

    def fail(x):
        raise ValueError(x)

    tpl = jinja2cpp.Template("{% for i in [1] %}{{ f(o) }}{% endfor %}")
    for _ in range(3):
        with pytest.raises(ValueError):
            tpl.render(o=obj, f=fail)
    gc.collect()
    before = sys.getrefcount(obj)
    for _ in range(LOOP):
        with pytest.raises(ValueError):
            tpl.render(o=obj, f=fail)
    gc.collect()
    assert sys.getrefcount(obj) == before


def test_environment_and_template_cycles_are_collected():
    env = jinja2cpp.Environment()
    # A cycle through a closure in env.globals and env.filters, and through the template
    env.globals["env"] = env
    env.filters["self"] = lambda v, env=env: env
    tpl = env.from_string("{{ 1|self }}")
    env.globals["tpl"] = tpl
    refs = [weakref.ref(env), weakref.ref(tpl)]
    del env, tpl
    gc.collect()
    assert all(r() is None for r in refs)


def test_loader_cycle_is_collected():
    class Loader(jinja2cpp.BaseLoader):
        def get_source(self, environment, template):
            self.env = environment
            return "x", None, None

    env = jinja2cpp.Environment(loader=Loader())
    env.get_template("a").render()
    ref = weakref.ref(env)
    del env
    gc.collect()
    assert ref() is None


def test_template_keeps_its_environment():
    tpl = jinja2cpp.Environment(loader=jinja2cpp.DictLoader({"a": "{% include 'b' %}", "b": "B"})).get_template("a")
    gc.collect()
    assert tpl.render() == "B"
    assert jinja2cpp.Template("{{ 1 }}").render() == "1"


def test_objects_kept_by_a_python_callable_outlive_the_render():
    kept = []
    tpl = jinja2cpp.Template("{{ keep(d, l, [1, 2], {'a': 'b'}) }}")
    tpl.render(keep=lambda *a: kept.append(a), d={"x": 1}, l=[1])
    gc.collect()
    assert kept[0] == ({"x": 1}, [1], [1, 2], {"a": "b"})


def test_keyboard_interrupt_stops_a_long_render():
    if threading.current_thread() is not threading.main_thread():
        pytest.skip("signals reach the main thread only")
    tpl = jinja2cpp.Template("{% for i in range(100000) %}{% for j in range(100000) %}{{ j }}{% endfor %}{% endfor %}")
    old = signal.signal(signal.SIGALRM, signal.default_int_handler)
    try:
        signal.setitimer(signal.ITIMER_REAL, 0.2)
        with pytest.raises(KeyboardInterrupt):
            tpl.render()
    finally:
        signal.setitimer(signal.ITIMER_REAL, 0)
        signal.signal(signal.SIGALRM, old)
    # The environment renders again afterwards
    assert tpl.environment.from_string("{{ 1 }}").render() == "1"


def test_render_from_threads():
    tpl = jinja2cpp.Template("{% for x in l %}{{ x }}{% endfor %}{{ d.k }}")
    results = []

    def work():
        for _ in range(50):
            results.append(tpl.render(l=[1, 2], d={"k": "v"}))

    threads = [threading.Thread(target=work) for _ in range(4)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    assert results == ["12v"] * 200


def test_classes_made_at_run_time_are_collected():
    # The module remembers per class whether it is an Undefined; that must not keep classes alive
    tpl = jinja2cpp.Template("{{ o }}{{ o.x }}{{ [o]|length }}")
    refs = []
    for i in range(200):
        cls = type(f"Made{i}", (), {"x": i, "__str__": lambda self: "o"})
        assert tpl.render(o=cls()) == f"o{i}1"
        refs.append(weakref.ref(cls))
        del cls
    gc.collect()
    assert all(r() is None for r in refs)
    # Classes that live on keep their answer: an Undefined subclass is still undefined
    class Missing(jinja2cpp.Undefined):
        pass

    for _ in range(3):
        assert jinja2cpp.Template("[{{ u }}]").render(u=Missing(name="u")) == "[]"


def test_loader_uptodate_closure_over_environment_is_collected():
    # get_source's uptodate callback is cached by the loader's native handler; a closure over the
    # environment makes a cycle environment -> handler -> closure -> environment
    class Loader(jinja2cpp.BaseLoader):
        def get_source(self, environment, template):
            return "x", None, lambda: environment is not None

    gc.collect()
    refs = []
    for _ in range(5):
        env = jinja2cpp.Environment(loader=Loader())
        assert env.get_template("a").render() == "x"
        refs.append(weakref.ref(env))
        del env
    gc.collect()
    assert [ref() for ref in refs] == [None] * 5
