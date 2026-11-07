"""The parity corpus (test/parity) through the module (docs/tasks/0355, 0248 plan D10.2).

Every case of ``test/parity/expected`` renders through ``jinja2cpp``, set up by
``generate.py``'s own code with jinja2cpp in the place of jinja2. Each outcome (match, output,
rejects, accepts, unsupported) must be the one the C++ ParityTest has for that case:
a match where ``divergences/<area>.txt`` lists nothing, the listed kind where it does. So the
module's pass count equals the C++ suite's, and a difference is a bug of the crossing
(conversion of the context, Python callables as filters, tests and finalize, the Environment
layer) unless ``module-divergences.txt`` lists it, with its owning task.
"""

import importlib.util
import json
import pathlib

import jinja2cpp
import pytest

REPO = pathlib.Path(__file__).resolve().parents[2]
PARITY = REPO / "test" / "parity"
MODULE_DIVERGENCES = pathlib.Path(__file__).resolve().parent / "parity" / "module-divergences.txt"


def _generate():
    spec = importlib.util.spec_from_file_location("parity_generate_module", PARITY / "generate.py")
    gen = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gen)
    # generate.py's case setup (make_env), with the module in jinja2's place
    gen.jinja2 = jinja2cpp
    gen.UNDEFINED = {k: getattr(jinja2cpp, v.__name__) for k, v in gen.UNDEFINED.items()}
    return gen


def _features():
    """The build features of divergence conditions (test/parity/README.md, "Build conditions")."""
    try:
        big = jinja2cpp.Environment().from_string("{{ 2 ** 100 }}").render() == str(2**100)
    except jinja2cpp.TemplateError:
        big = False
    return {"bigint"} if big else set()


def _applies(kind, features):
    """The kind of a divergence line without its condition, or None where the condition fails."""
    kind, _, condition = kind.partition("@")
    if not condition:
        return kind
    negated = condition.startswith("no") and condition[2:] in {"bigint"}
    feature = condition[2:] if negated else condition
    return kind if (feature in features) != negated else None


def _read_list(path, features):
    listed = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        case_id, kind, task, *_ = line.split(None, 3)
        kind = _applies(kind, features)
        if kind is not None:
            listed[case_id] = (kind, task)
    return listed


def _cpp_divergences(features):
    """The narrow-template lines of divergences/*.txt: the C++ ParityTest's expected outcomes."""
    listed = {}
    for path in sorted((PARITY / "divergences").glob("*.txt")):
        for case_id, entry in _read_list(path, features).items():
            # wide.* are the wide-template variants; reflect.* the view pass (0322): not this path
            if not case_id.startswith(("wide.", "reflect.")):
                listed[case_id] = entry
    return listed


def _outcome(gen, case):
    try:
        got = gen.render(case)
    except NotImplementedError:
        return "unsupported"  # an Environment option the module refuses (UnsupportedError)
    if "error" in case:
        return "match" if "error" in got else "accepts"
    if "error" in got:
        return "rejects"
    return "match" if got["output"] == case["output"] else "output"


def _expected(listed, case_id):
    entry = listed.get(case_id)
    return entry[0] if entry else "match"


def test_corpus_through_module():
    gen = _generate()
    features = _features()
    cpp = _cpp_divergences(features)
    module = _read_list(MODULE_DIVERGENCES, features)
    cases = []
    for path in sorted((PARITY / "expected").glob("*.json")):
        cases += json.loads(path.read_text(encoding="utf-8")).get("cases", [])
    ids = {case["id"] for case in cases}
    assert not set(module) - ids, f"module-divergences.txt lists unknown cases: {sorted(set(module) - ids)}"

    matches = cpp_matches = 0
    problems = []
    for case in cases:
        case_id = case["id"]
        cpp_kind = _expected(cpp, case_id)
        cpp_matches += cpp_kind in ("match", "platform")
        if cpp_kind == "crash":
            continue  # never run: the C++ suite skips it too
        outcome = _outcome(gen, case)
        matches += outcome == "match"
        want = _expected(module, case_id) if case_id in module else cpp_kind
        if want == "platform" and outcome in ("match", "output"):
            continue
        if outcome != want:
            where = "module-divergences.txt" if case_id in module else "divergences/"
            problems.append(f"{case_id}: module {outcome}, {where} says {want}")
    print(f"\nparity corpus through the module: {matches} of {len(cases)} cases match "
          f"(C++ ParityTest: {cpp_matches} with features {sorted(features)}; "
          f"{len(module)} listed in module-divergences.txt)")
    assert not problems, "the module and the C++ suite disagree:\n" + "\n".join(problems)


@pytest.mark.parametrize("line", ["calls.x output 0355 reason", "calls.x rejects@nobigint 0178 reason"])
def test_list_format(tmp_path, line):
    path = tmp_path / "list.txt"
    path.write_text(f"# comment\n{line}\n", encoding="utf-8")
    assert _read_list(path, set()) == {"calls.x": (line.split()[1].partition("@")[0], line.split()[2])}
