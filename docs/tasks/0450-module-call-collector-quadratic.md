---
status: done
priority: medium
area: robustness
depends: [0444]
touches: [src/template_module.cpp, src/cycle_collector.cpp]
shares: [test/template_module_test.cpp]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/829
---
# A module whose calls grow a module-level container walks all of it on every call

**Problem.** A template module collects cycles at the end of every host call
(`ModuleCallCollector`). A container at the module's top level that a call stores into is noted,
stays a candidate (the module state holds it, so its count is above the references the walk
finds) and is walked again by every later call that stores into it: N calls cost O(N²).
Measured by the security review of PR #822 on a Debug build, with
`{% set cache = [] %}{% macro m(a) %}{% set _ = cache.append(EXPR) %}{% endmacro %}` called N
times with a dict argument:

| EXPR | N=2000 | N=4000 | N=8000 |
|---|---|---|---|
| `1` | 41 ms | 93 ms | 181 ms |
| `[1]` | 1.4 s | 5.2 s | 23 s |
| `a.name` | 2.9 s | 11.5 s | 42.7 s |

`[1]` was quadratic before #822. #822 (0444) also notes the store of a value whose object is
traversable, such as a string read from a lent map, which carries an anchor
(`SetParentData`), so `a.name` became quadratic too. After Codex's review #822 also stopped
anchoring values that own their data (numbers, bool, none, undefined, inline and owned strings):
`a.name` and `a.n` are linear again (4000 calls: 82 ms and 78 ms). What remains quadratic is a
stored list or dict (`[1]`, 4000 calls: 5.8 s) and a string that only views its container's text. Within one render the cost is amortised
(each container is noted once, candidates are pruned), so only module calls are hit. A host that
calls an exported macro in a loop (a chat-template server keeping a log in the module) is the
case to fix.

**Proposal.** Keep what a call's walk found reachable from the module state from being walked
again unless a later call writes to it: a per-module "known reachable" generation, or note in
the module's collector only the containers the call itself made or changed since the last walk.
Alternatively, skip the walk of a candidate the module state holds directly (its top-level
names) when the call stored only values that cannot reach the call's own new objects.

**Done when** the table above is linear in N for `[1]` and for a stored view string (a
`TemplateModuleTest` row with a step-count or time budget), and the cycle cases in
`ExpressionTest.CyclesThroughObjects` and `TemplateModuleTest` still free their cycles under
LeakSanitizer.

**Result (PR for 0450/0263/0265).** The walk at the end of a module call now takes the objects the
module's own names hold, and those held by the attributes of a namespace among them, as held from
outside without walking into them (`ModuleScope::LiveRootsFn` in src/cycle_collector.h,
`TemplateModuleState::LiveRoots` in src/template_module.cpp): nothing they reach is garbage, so
the result of the walk is unchanged, and a call that stores into such a container walks only what
it made. Measured on a Debug build, 1000 against 4000 calls, before and after:

| Stored | before | after |
|---|---|---|
| `cache.append([1])` | 0.31 s / 4.7 s | linear |
| `cache.append({'n': a.name})` | 0.37 s / 5.4 s | linear |
| `cache.append(a.items)` | 1.0 s / 21 s | linear |
| `cache.append(a.items[0])` (view string) | 1.0 s / 18.8 s | linear |
| `cache.update({k: [a.n]})` | 0.40 s / 5.8 s | linear |
| `ns.log.append([a.n])` | 0.46 s / 5.2 s | linear |

Pinned by `TemplateModuleTest.StoresIntoModuleContainersStayLinear` (4000 calls under 9 times
1000). A container deeper than a top-level name or a namespace attribute (`cache.log.append(...)`
with `cache` a dict) is still walked per call. Found on the way: `d|length` of a module's
exported dict built the key list on every call; fixed with 0263's `|length` change.
