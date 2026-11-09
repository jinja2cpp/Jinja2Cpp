---
status: in-progress
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

**Plan (approved by Ruslan 2026-11-06).** [docs/design/0357-python-module-public-api.md](../design/0357-python-module-public-api.md): a seam inventory, public additions B1-B7, PR steps S0-S4, and decisions D1-D7, all taken as recommended (D6b: public `ISandboxPolicy`).

**Done when.** The module compiles with only `include/jinja2cpp/` on its include path, from
an installed package. `src/host_object.h` is gone or used only by the engine. The upstream
pass rate and the bench row match the values before the switch. This lands before the G1
freeze.

**Progress.**
- S0 baseline on master be676f1 (2026-11-06, Python 3.13, Release module with boost big ints):
  `python/tests` 236 passed; the parity corpus through the module
  (`/mnt/project-files/0357/corpus_module.py`) matches 3144 of 3225 cases (19 output, 3 rejects,
  1 accepts, 58 options the module refuses, mostly i18n); `python/bench/bench_module.py` (min
  time 1 s, one run) wins Load and Render on all 25 cases, the narrowest Render margins are
  expressions and chat_llama at 1.5x. Raw files in `/mnt/project-files/0357/s0-*`.
- S1a and S2a: the public `jinja2cpp/host_object.h` and its engine bridge
  (`src/public_host_object.{h,cpp}`), export views (tuple flag, stable identity, original dict
  keys), callable members of a host call's template containers lent for the call, opt-in
  `keepUndefined`, `UserCallableParams::site`, the context's keys, `BigInt::FromDigits`/`ToDigits`.
  The private seam is renamed `IEngineHostObject`; the module still uses it until S3.
- For S1b: `MakeHostValue(shared_ptr)` costs two allocations besides the object (the bridge and
  the `shared_ptr` control block, as the engine holds it by intrusive reference and `GenericMap`
  by `shared_ptr`), and `MakeHostValue<T>` three; the probe decides whether one ownership model
  must give way. Follow-ups filed: 0421 (a tuple a host returns), 0422 (exports of a running
  module call), 0423 (callables nested deeper in a host call's argument).
- 2026-11-09, Ruslan's decision on D2: option (c) plus one more sink, residual gap accepted.
  The branch is rebased on master after #794, #801 and #805 (the sandbox policy's comment now
  names `IEngineHostObject`). Taken: one allocation per host value with no `dynamic_cast` on the
  host path, direct indexing of random-access host lists, `HostResult` with `GetAttrTo`,
  `GetItemTo`, `ItemAtTo`, and `HostResult::SetHostObject` (shared or built in place) for nested
  host objects, with tests in `test/public_host_object_test.cpp`. Callgrind instructions per
  render through the probe module (PyHost on the public interface, nested dicts, lists and tuples
  through the sink; throwaway branch `wt-0357-sink`), second of two runs, private seam / (c) /
  (c) + sink: for_filter_if 818,610 / 894,552 / 853,131; chat_llama 636,918 / 726,761 /
  710,482; mitsuhiko_table 8,211,299 / 9,131,276 / 8,890,719. D7 is relaxed for host-data cases
  (api-2.0.md section 7 item 10); `detail::MakeHostValueInPlace` and
  `HostResult::SetHostObjectInPlace` are frozen ABI.
- S2b (B3, B6), stacked on #800: `RenderOptions` (`globals` as `shared_ptr<const ValuesMap>`, so
  a value a callable keeps outlives the render as an environment global does through
  `LendGlobal`; `poll` as `std::function<void()>`) with `Template::RenderTo(out|sink, params,
  options)` over the `RenderCore<true>` instantiation; a poll alone makes the render count its
  steps, and a render a polled one starts (a callable rendering another template) is polled too.
  `Template::FindReferencedTemplates()` is public, and `test/referenced_templates_test.cpp` now
  builds in shared mode. Found on the way: in a wide template every referenced name came back as
  `nullopt` (the walk read narrow strings only); fixed, with a row. The module's poll moved to
  the `std::function` form of `detail::t_hostPoll`; its render entry is unchanged until S3.
