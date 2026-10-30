---
status: open
priority: medium
area: api
depends: [0353, 0212]
touches: [python/src/, src/host_object.h]
shares: [include/jinja2cpp/value.h, include/jinja2cpp/user_callable.h, docs/api-2.0.md]
---
# Python module: move from the private seam to the public API before the G1 freeze

**Problem.** The module is built on the private engine seam from 0352 (0248 plan, D3).
That is fast to build, but it is a second contract next to the public API. Each internal
refactor must then keep the module working, and a capability that only the module has is a
capability C++ users cannot get. Ruslan (2026-10-30) wants the project's obligations reduced
to the public interface: the module must sit on public API only, settled just before the
freeze, so that we do not take on more obligations.

**Proposal.** When the API freeze (G1) approaches, list every seam operation the module
uses. For each one, either:
- point it at an existing public API: GenericMap and GenericList, UserCallable with the #645
  call protocol, 0049's identity round trip, or `JsonTraits` for snapshots; or
- design the smallest public addition that covers it, for example a public host-object
  accessor that also gives C++ users 0212's object identity and printing. That addition is
  recorded as an API decision in `docs/api-2.0.md` and lands in the API wave.
Then switch `python/src/` to the public API, build the module against the installed public
headers only, and delete `src/host_object.h`, or reduce it to an implementation detail of the
public interface. The perf gate of 0353 and the pass rates of 0355 must not regress.

**Done when.** The module compiles with only `include/jinja2cpp/` on its include path, from
an installed package. `src/host_object.h` is gone or used only by the engine. The upstream
pass rate and the bench row match the values before the switch. This lands before the G1
freeze.
