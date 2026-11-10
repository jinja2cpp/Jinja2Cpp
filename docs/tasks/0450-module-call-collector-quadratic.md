---
status: open
priority: medium
area: robustness
depends: [0444]
touches: [src/template_module.cpp, src/cycle_collector.cpp]
shares: [test/template_module_test.cpp]
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
(`SetParentData`), so `a.name` became quadratic too. Within one render the cost is amortised
(each container is noted once, candidates are pruned), so only module calls are hit. A host that
calls an exported macro in a loop (a chat-template server keeping a log in the module) is the
case to fix.

**Proposal.** Keep what a call's walk found reachable from the module state from being walked
again unless a later call writes to it: a per-module "known reachable" generation, or note in
the module's collector only the containers the call itself made or changed since the last walk.
Alternatively, skip the walk of a candidate the module state holds directly (its top-level
names) when the call stored only values that cannot reach the call's own new objects.

**Done when** the table above is linear in N for all three expressions (a
`TemplateModuleTest` row with a step-count or time budget), and the cycle cases in
`ExpressionTest.CyclesThroughObjects` and `TemplateModuleTest` still free their cycles under
LeakSanitizer.
