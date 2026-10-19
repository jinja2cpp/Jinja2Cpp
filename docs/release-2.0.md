# Jinja2C++ 2.0: release gates and RC signoff

This is the one place that says what 2.0 ships, what blocks the tag, what is postponed
and why, and how a release candidate (RC) is approved (docs/tasks/0221). The
API itself is designed in [api-2.0.md](api-2.0.md); individual fixes stay in their task
files, and this document links to them rather than repeating them.

A green parity suite means "no difference outside the recorded divergences". It is not
a claim of complete Jinja2 compatibility, nor of safety for untrusted templates. Both
claims the release does make are bounded in sections 5 and 3 (G3) below.

**Scope rule (Ruslan, 2026-10-18).** 2.0 takes every open task in `docs/tasks/` by default.
Releasing before that has no point. A task stays out only if it is on the postponed list of
section 5, with the reason it cannot be part of the release.

## 1. Status

| | |
|---|---|
| Release manager and approver | Ruslan (`rmorozov`) |
| Version | 2.0.0, SOVERSION 2 (already set: `CMakeLists.txt`, `project(... VERSION 2.0.0)` and `SOVERSION 2`) |
| Freeze | not yet; the API freeze waits on G1 (with the API wave of section 3) and the decisions in section 8 |
| Candidate commit | **none yet**. The RC is recorded here as a full SHA when it is cut (section 7) |
| Reference commit for this revision | master `b0fb0ef` (2026-10-18); GitHub issues, PRs and milestones 3/7 scouted the same day (section 6) |
| Last observed CI on master | `e69c8ad` (#561): linux, macOS, Windows, Conan, CodeQL and benchmark workflows all passed; `345d0c6` changed only `docs/` and ran the format check alone |

"Configured" below means a CI job exists; "observed" means a passing run on a named
commit. Only observed runs on the candidate commit count for signoff.

## 2. Scope of 2.0

2.0 ships master plus **every task open in `docs/tasks/` on the freeze date**, except the
postponed list in section 5. On 2026-10-18 that is 69 open tasks (this one included), among them:

- The 2.0 public API of api-2.0.md (`Value` accessors, `TemplateEnv`/`Settings` renames,
  containers and reflection, ordered `ValuesMap`, `Result<T>` on pinned expected-lite,
  `[[nodiscard]]`, unknown filters/tests as Load errors, v1 names as `[[deprecated]]`
  forwards), the inline ABI namespace `jinja2::v2` (0076) and migration (0077, 0222).
- Everything that shapes the public API, so that it is decided once before the freeze:
  one error contract for the whole public API (0250, Ruslan 2026-10-18) and one allocator and
  memory contract (0252), which the others follow, implemented by 0253 (error contract P1),
  0262 (`Settings::Limits`) and 0266 (`ConfinedFileSystem`),
  unsigned values (0172), big integers behind a backend interface (0178), Markup from C++
  (0180), name-based autoescape (0181), non-string mapping keys (0036), resource and
  recursion limits (0097, 0098), `[[nodiscard]]` on `Load`/`Render` (0080), the C++-only
  filters (0233), template introspection (0236).
- Every open parity task (0039, 0041, 0045, 0049, 0107, 0203, 0209-0216, 0219, 0220, ...),
  the oracle policy (0186) and the parity.md cleanup (0235).
- Correctness and robustness: 0110, 0183, 0223, 0224, structure-aware fuzzing (0095),
  ClusterFuzzLite (0096, in-repo part).
- Dependencies and build: Boost-free core (0230, 0229), robin_hood successor (0231), pin
  drift (0232), SBOM (0187), MSVC items (0079, 0174, 0238), GCC 13 sanitizer build (0184),
  Bazel build (0245, build part), CI speed (0162), coverage gate (0010), Nix toolchains (0078).
- Performance and reach: a public comparison with rival engines (0246), their benchmark
  cases and a perf round with an exit criterion (0247), a drop-in Python module (0248).
- Distribution and docs: licensing (0226), source-artifact check (0225), Conan recipe
  (0006), supported docs and examples (0227), the website (0239-0244), tracker cleanup (0234).
- C++17 as the language floor; C++17, 20 and 23 supported and tested.

### Supported platforms (configured)

| Platform | Compilers | Standards | Linkage | Where |
|---|---|---|---|---|
| Linux | GCC 12, 14; Clang 18, 20 | 17, 20, 23 (26: forward-compat check only) | static, shared | `linux-build.yml` (12+2 jobs, pairwise) |
| Linux, sanitizers | Clang 18 ASan+UBSan, GCC 14 ASan+UBSan, Clang 18 TSan | 17 | static | `linux-build.yml` |
| Linux, deps modes | internal, external, conan-build | 17 | static, shared | `linux-build.yml`, `conan-build.yml` |
| Windows | MSVC (windows-latest) x64 | 17, 20, 23 (`/std:c++latest`) | /MT, /MD, DLL | `windows-build.yml` |
| macOS | AppleClang, macOS 14 and 15 | 17, 20, 23 | static | `macos-build.yml` |

JSON bindings: boost (default), nlohmann, rapid, all in the Linux matrix. "Supported" for
2.0 means this table; anything else is best effort. D9 asks to confirm it.

## 3. Gates

A gate passes when its evidence is observed on the candidate commit. **B** = blocks the
tag; **A** = accepted for 2.0, tracked, does not block.

| Gate | What must hold | Tasks | Owner | B/A | Evidence |
|---|---|---|---|---|---|
| G0 Scope | Every task in `docs/tasks/` is `done`, `dropped`, or on the postponed list of section 5; nothing is postponed without Ruslan's word | all open tasks | Ruslan | B | `grep -l 'status: open' docs/tasks/0*.md` on the candidate, checked against section 5 |
| G1 API/ABI freeze | Inline namespace `v2` in every public header, `fwd.h`, no leaked macros or vendored names; `nm` shows `jinja2::v2::`; a consumer built against 1.3.2 headers fails to link against 2.0 (static and shared); the **API wave** has landed first: 0250 (one error contract for the public API) and 0252 (allocator and memory contract, PR #575) settle before the rest, then 0253 (error contract P1), 0262 (`Settings::Limits`), 0266 (`ConfinedFileSystem`), 0172, 0178, 0180, 0181, 0036, 0097, 0098, 0080, 0233, 0236, 0183 (every task that adds to or changes public headers) follow it; decisions of section 8 recorded | [0076](tasks/0076-abi-namespace-and-headers.md) and the API wave | 0076 thread (running) | B | 0076's tests in CI; negative link check |
| G2 Known correctness defects | `*context` callable binds positional args; self-referencing namespace/dict/list does not leak under LSan; unsigned above `INT64_MAX` renders the value or fails loudly | [0183](tasks/0183-context-callable-positional-arg.md), [0110](tasks/0110-namespace-self-reference-leak.md), [0172](tasks/0172-value-unsigned-overflow.md) | one thread each (started 2026-10-18) | B | their unit rows; sanitizer jobs with `detect_leaks=1` |
| G3 Embedding and trust contract | Lifetime/cache/callback scenarios pass under ASan/UBSan/LSan and TSan; `docs/security.md` states the trust model, what `RealFileSystem` does with `../` and absolute names, the fixed caps, and a reporting contact | [0223](tasks/0223-embedding-lifecycle-integration-tests.md), [0224](tasks/0224-template-trust-and-loader-boundary.md) | unassigned | B | sanitizer jobs; 0224's disposable-file probes |
| G4 Migration | `MIGRATION.md` covers renames and behaviour changes with before/after; the migration script rewrites our 1.x-style tests warning-free in CI; a 1.3.2 consumer fixture upgrades against the installed RC | [0077](tasks/0077-migration-script.md), [0222](tasks/0222-downstream-1x-migration-trial.md) | unassigned (after G1) | B | CI job of 0077 and 0222 |
| G5 Distribution | License conclusion recorded and LICENSE/notices installed; the source archive builds with tests off in a clean directory and a separate consumer uses it via `find_package` and pkg-config; Conan recipe validated with an installed-package consumer | [0226](tasks/0226-license-metadata-and-distribution.md), [0225](tasks/0225-release-artifact-consumer-gate.md), [0006](tasks/0006-conan-ci-and-releases.md) | unassigned | B (publication: postponed, section 5) | 0225's check output with archive hash; Conan consumer job |
| G6 Docs and examples | README, `MIGRATION.md` and the versioned 2.0 site agree on C++17, CMake ≥ 3.23 and dependencies; every advertised snippet and example builds against the RC; historical repositories labeled. The website (jinja2cpp.github.io) builds from pinned versions in CI, serves 1.x and 2.x side by side, has a CI-generated 2.0 API reference, and no 2.x page contradicts `docs/parity.md` | [0227](tasks/0227-supported-docs-and-example-surface.md) (+ README via 0077), [0239](tasks/0239-site-build-and-deploy.md), [0240](tasks/0240-site-versioned-docs.md), [0241](tasks/0241-api-reference-in-ci.md), [0242](tasks/0242-site-content-refresh.md) | unassigned | B | `docs/project-support.md` validation table; site CI build, link check and snippet compile on the RC tag |
| G7 Parity | The corpus passes on the candidate with no `crash` and no `unsupported` entries; every remaining divergence line names an open task or is `deliberate:`; every ❌/🟡 feature row of `docs/parity.md` cites an open task; the deliberate list (section 5) is current | [0235](tasks/0235-parity-feature-rows-drift.md) | merge steward | B | `ctest -R parity`; `parity-expectations` workflow |
| G8 Robustness | Full CI green incl. sanitizer jobs; the nightly fuzz workflow passes on the candidate (dispatch it); an extended structure-aware fuzz campaign (0095: custom mutator and grammar-generated differential run) on the candidate finds no new crash, leak or silent wrong value; `fuzz/regressions/` replays clean | [0095](tasks/0095-structure-aware-fuzzing.md) | merge steward | B | observed runs on the candidate |
| G9 Performance | The benchmark workflow passes against the previous master (no regression past its gate). No further gains required | — | perf track | B | `benchmark.yml` run on the candidate |
| G10 Inventory | `scripts/sbom.py --check` and `scripts/task_index.py --check` pass; the dependency inventory matches what the release installs | [0187](tasks/0187-generated-data-acquisition.md) (SBOM stays there) | merge steward | B | format-check job |
| G11 Tracker | Every open GitHub issue names an open task or is closed with a pointer to its fix; milestones 1.2 and 1.3 closed; external PRs #284, #199, #197 resolved; done in the pre-freeze sweep (section 7, step 0) | [0234](tasks/0234-github-tracker-cleanup.md) | unassigned | B | issue list on the RC date |
| G12 Hash map successor | Vendored `robin_hood.h` (upstream archived) replaced by the successor the perf track picks, with its two local additions ported; no regression past the Cachegrind gate; `sbom/components.json` matches | [0231](tasks/0231-robin-hood-successor.md) | perf track | B (Ruslan 2026-10-18) | bench/ and Cachegrind gate results on the switch PR |
| G13 Comparison report | A generated benchmark and feature-parity report against Python Jinja2, MiniJinja, Tera, minja and inja is published for the RC; rival suites' cases are covered; 0247's perf exit criterion holds or each exception is recorded; the Python drop-in module's pass rate is in the report | [0246](tasks/0246-public-comparison-report.md), [0247](tasks/0247-rival-benchmark-coverage.md), [0248](tasks/0248-python-drop-in-replacement.md) | perf track | B (Ruslan 2026-10-18) | report generated on the candidate; `bench/run.py` output |

Dependency order: 0076 (running) → the API wave → G1 freeze → G4 (0077 → 0222); 0226 → 0225; 0227 alongside 0077. G2, G3 and
G5's 0226 can start now. 0230 (Boost-free core) and 0231 go before G5's validation, so that 0226, 0225, 0006
and 0245 validate one dependency set. Website: 0239 → 0240 →
0241, 0242.

## 4. Compatibility promises

From api-2.0.md 5.3-5.4, stated as the contract users get:

- **Source.** 2.0 breaks 1.3.2 source where api-2.0.md lists it; v1 names that could be
  kept compile with `jinja2cpp-2:` deprecation warnings that the migration script reads.
  Within 2.x, public names are only added or deprecated, never removed (D5 says when
  deprecated names go).
- **ABI across majors.** The inline namespace `jinja2::v2` makes 1.x headers against a
  2.x library (or the reverse) a link error, for static and shared builds alike.
- **ABI within 2.x.** SOVERSION tracks the ABI, not the major. It is 2 for 2.0.0 and is
  bumped by any minor release that changes an exported layout, including adding a
  `Settings` field (user code allocates `Settings`). A 2.x minor is therefore **not**
  promised to be binary compatible; a patch release is. Static users rebuild on any
  minor upgrade.
- **Behaviour.** Template output may change in minor releases where it moves towards
  the oracle; such changes are listed in the release notes. Deliberate divergences
  (section 5) change only by a decision recorded in their task.
- **Markup values (0180).** `Value::ValueData` gained the alternatives `jinja2::Markup` and
  `jinja2::WMarkup` (immutable wrappers of `std::string`/`std::wstring`, `include/jinja2cpp/markup.h`),
  appended at indices 13 and 14; `sizeof(Value)` is unchanged. `IsString()`, `AsString()`,
  `As<std::string>()` and `GetIf<std::string>()` accept a Markup (mutable access to the text
  of a Markup value turns the value into a plain string first, as any change to a Python
  `Markup` gives a `str`; a Markup has no `append` or `+=`), but code that visits
  the raw variant (`std::visit(..., v.GetData())`, `std::get<std::string>`,
  `std::holds_alternative<std::string>`) sees a Markup as its own alternative: a visitor
  with a generic overload now receives it there, and one without an overload for it no
  longer compiles. Host `==` compares structurally (`Value(Markup("a")) != Value("a")`);
  templates still compare the text. To change a Markup, build the new text from `str()` and
  wrap it again, which states that it is safe.
- **Dependencies in the interface.** `Result<T>` stays `nonstd::expected` (pinned) for
  all of 2.x; moving to `std::expected` needs a C++23 floor and is a 3.0 question.

## 5. Postponed, and why

Only these stay out of 2.0. Each reason is about the kind of work, not its size: it either
cannot happen before a tag exists, or it has no end state a release could wait for.

| What | Tasks | Why it cannot be in the release |
|---|---|---|
| Publishing to package registries: ConanCenter, the Bazel Central Registry, a vcpkg port | publication parts of 0006, 0245; vcpkg (0006/0227) | A registry publishes a tagged release, and its review is outside the project. The recipes and their consumer checks are in scope (G5); publication follows the tag |
| OSS-Fuzz enrollment | 0096 (OSS-Fuzz part) | Acceptance is Google's decision. ClusterFuzzLite, which runs in our own CI, is in scope |
| Recurring processes | 0123 (weekly perf scout, drift alert), 0099 (nightly differential triage), 0228's ongoing use | They run for as long as the project does; the release takes their state at the RC. Any finding they file before the freeze is in scope by the rule above |
| Performance work beyond 0247's exit criterion | whatever 0164, 0117 and 0204 hold after the pre-2.0 perf round | Ruslan (2026-10-18) brought a perf round into 2.0, driven by the comparison report: every case on par with the fastest rival or a recorded reason (0247). Past that point there is no done state, and internal speedups change no API or ABI |
| Publishing wheels to PyPI | publication part of 0248 | Like the other registries, it follows the tag. The drop-in module itself, its wheels built in CI and its test-suite pass rate are in scope (0248) |
| Parity cases found after the freeze | — | The corpus keeps growing; new divergences found during the RC window are filed for 2.x unless they are crashes, leaks or silent wrong values, which block (G8) |

**Deliberate divergences** stay documented, not bugs (14 corpus lines): integer overflow
raises in the default build (0015; big integers are opt-in through 0178's backend),
names captured by a closure that outlives its frame read as undefined (0165, 6 lines),
`select`/`map`/`reject` results can be iterated twice (0101, 2 lines), `%c` of a lone
surrogate in wide templates (0106), a call block returning a dict prints it (0156).
Corpus on master `b0fb0ef`: 1929 cases, 1886 match.

## 6. Confirmed facts for this revision

- **0179 is closed by #556.** Merged in train 20 (`1dd4fd6`, then `9ec94e3`); 0179 has
  `status: done` with the Done note; `autoescape.env_on_caller_markup_call_site` has no
  divergence line and `Parity/ParityTest.MatchesPython/autoescape_env_on_caller_markup_call_site`
  passes on `345d0c6` (local Debug build, all 127 autoescape cases pass). The only
  remaining autoescape line is `volatile_macro_output`, owned by 0216.
- 0183 is reproduced (review of 2424f52): `ContextArgFn('test_value')` renders `none`
  next to the keyword row at `test/user_callable_test.cpp:342`.
- `README.md:187` still says `find_package(jinja2cpp 1 REQUIRED)` (G6).
- Boost.Multiprecision supports `BOOST_MP_STANDALONE` (its README: the only dependency is
  Boost.Config, both header-only), checked in the fetched Boost tree; relevant to D3.

- **GitHub scout (2026-10-18, master `b0fb0ef`).** All 49 open issues, the external PRs and
  milestones 3 ("Release 1.2") and 7 ("Release 1.3") were checked against the backlog, with
  each repro run on master and Python Jinja2 3.1.6 as the oracle. About 35 issues are fixed
  on master and only wait to be closed (0234). Uncovered findings were filed as 0233
  (C++-only filters registered but empty, #41), 0234 (tracker), 0235 (stale parity.md rows),
  0236 (introspection), 0237 (Python bindings, #256), 0238 (MSVC debug info, #224). Both
  milestones are delivered by master except `flip` (obsolete: Jinja2 has no such filter),
  the vcpkg port (0006) and the C++-only filters (0233).
- **Website scout (same day, site master `dccc3f0`).** Classic Pages build on an unpinned
  remote theme, no CI, a hand-made 1.0 Doxygen tree from 2019, a compatibility page that
  contradicts parity.md, examples that no longer compile (`reflection.md`). Filed as
  0239-0244; 0227 and 0077 keep the support policy, build guide and migration text.

## 7. RC procedure

0. **Tracker sweep (Ruslan 2026-10-19).** Right before the freeze, scan once more through
   every open GitHub issue, every milestone and every open PR, and draft one list of what
   to close: issues fixed on master (with a pointer to the fix), issues and PRs that are
   stale or no longer relevant (with a one-line reason), and milestones that are delivered
   or superseded. Ruslan approves the list, then it is closed in one pass (0234). Until
   this sweep, external PRs and issues are left as they are.
1. **Freeze.** When G1-G6 are done and section 8 is decided, the steward picks a master
   commit, records its full SHA in section 1 and opens the validation window. From then
   on master takes only fixes for blocking findings; other merges wait.
2. **Validate (window: D11).** On the candidate commit, observe and record here, with run
   links: every build workflow, sanitizers, `parity-expectations`, a dispatched `fuzz` run, the extended structure-aware fuzz campaign (G8),
   `benchmark`, CodeQL, the 0225 artifact check (archive SHA-256, toolchains, dependency
   versions), the 0222 upgrade trial, the 0006 Conan consumer, and the 0227 snippet checks.
   Locally reproducible core:

   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
   ctest --test-dir build --output-on-failure
   cmake -S . -B build-asan -G Ninja -DJINJA2CPP_WITH_SANITIZERS=address+undefined \
         -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build-asan && \
         ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-asan
   python test/parity/generate.py --check        # under the oracle Python (D7)
   python3 scripts/sbom.py --check && python3 scripts/task_index.py --check
   ```

3. **Changes after the freeze** produce a new candidate SHA. Gates whose inputs the change
   touched run again: any `src/`/`include/` change reruns G2, G3, G7, G8, G9 and the
   platform builds; packaging or CMake changes rerun G5; doc changes rerun G6's snippets.
4. **Waivers.** A failing blocking gate is either fixed or waived by Ruslan in writing in
   this file (gate, reason, task that tracks it, expiry release). A waiver moves the item
   to section 5's postponed list; nothing is waived silently or by an agent.
5. **Signoff.** Ruslan approves the candidate SHA; then: tag `v2.0.0` on that SHA, publish
   the source archive whose hash 0225 recorded, the release notes (`MIGRATION.md` plus
   section 5), the versioned docs site; then submit the postponed registry publications
   (ConanCenter, BCR) for the tag. The tag commit
   must equal the validated SHA.

## 8. Decisions for Ruslan

All decided on 2026-10-18 except D19, which Ruslan deferred to the end of the backlog.
Answers are recorded here and in the linked task.

| # | Question | Options | Recommended | Why |
|---|---|---|---|---|
| D1 | `Value(uint64_t)` above `INT64_MAX` (0172) | *Decided by Ruslan 2026-10-18:* a) throw from `Value`/`Reflect`, for now | — | b loses precision silently, c costs every evaluator path. Confirmed by the error contract (0250, approved 2026-10-18 12:24): kept, typed as `jinja2::Exception{IntegerOutOfRange}`; 0172 follows 0250 |
| D2 | Big integers in 2.0 (0178) | *Settled by the scope rule (2026-10-18):* in 2.0 | — | The backend interface of D3 touches `Value`, so it belongs before the freeze |
| D3 | 0178 (big integers) against 0230 (Boost-free core) | *Decided by Ruslan 2026-10-18:* a big-integer backend interface. The default build has none (int64, overflow raises); an optional compile-time backend over Boost.Multiprecision `cpp_int` is the sample implementation; users may supply their own | — | Keeps the core Boost-free and makes big integers opt-in; the architect plan in 0178 decides compile-time against run-time backends |
| D4 | Markup on host values (0180) | *Settled by the scope rule:* in 2.0, in the API wave | — | It changes `Value`; doing it before the freeze avoids a SOVERSION bump right after 2.0 |
| D5 | When deprecated v1 names are removed | *Recommendation stands (no objection, 2026-10-18):* a) kept for all of 2.x, removed in 3.0 | — | Matches the source promise of section 4 |
| D6 | `[[nodiscard]]` on `Template::Load`/`Render` (0080) | *Recommendation stands (no objection, 2026-10-18):* a) in 2.0 | — | Adding it later is a new warning, an error under `-Werror` |
| D7 | Oracle pair (0186) | *Decided by Ruslan 2026-10-18:* a) one pinned pair per release (now Python 3.12 + Jinja2 3.1.6) as the default | — | Ruslan wants a separate design pass on CMake machinery that lets a build target a Jinja2 version other than the bundled one (0186) |
| D8 | Conan publication as a gate (0006) | *Recommendation stands (no objection, 2026-10-18):* a) RC needs a validated recipe and an installed-package consumer; ConanCenter publication follows the tag | — | Publication depends on ConanCenter reviewers |
| D9 | Supported platforms | *Recommendation stands (no objection, 2026-10-18):* a) the table in section 2 | — | It is what CI observes |
| D10 | Volatile `{% autoescape %}` (0216) | *Settled by the scope rule:* in 2.0 | — | |
| D11 | RC validation window | *Decided by Ruslan 2026-10-18:* a) 7 days with no blocking finding, for now | — | Ruslan puts the weight on fuzzing instead: G8 requires an extended structure-aware fuzz campaign (0095) on the candidate |
| D12 | Resource and recursion limits (0097, 0098) and `select_autoescape` (0181) | *Settled by the scope rule:* in 2.0, in the API wave | — | New `Settings` fields land before the freeze, not as 2.x SOVERSION bumps |
| D13 | Non-string mapping keys (0036) | *Decided by Ruslan 2026-10-18:* b) template-internal only; public keys stay `std::string` unless 0036's plan shows a public need | — | Fixes the parity cases without changing the public map |
| D14 | 1.3.2 consumer for 0222 | *Recommendation stands (no objection, 2026-10-18):* a) a self-contained fixture in `test/migration/` | — | Runs in CI; a real downstream project can follow |
| D15 | Bazel (0245) | *Settled by the scope rule:* the Bazel build and its CI are in 2.0; BCR publication follows the tag (section 5) | — | |
| D16 | C++-only filters that do nothing (0233) | *Decided by Ruslan 2026-10-18:* a) implement `camelize`, `underscorize`, `escapecpp`, `toyaml` with documented output and remove `toxml` | — | JSON-like data has no single XML mapping |
| D17 | Boost-free core (0230) | *Settled:* in 2.0, before the distribution checks (G5) | — | Ruslan 2026-10-18 |
| D18 | Website reference pages (0243) and playground (0244) | *Settled by the scope rule:* in 2.0 | — | |
| D19 | What happens if, at the end, only tasks that change neither API, ABI nor rendered output are left (CI and tooling: 0010, 0078, 0162; internal: 0174, 0184, 0204, 0232) | *Deferred by Ruslan 2026-10-18:* decided when the tag is close and the 2.0 backlog is almost empty, with the list in hand | — | Options then: a) they hold the tag; b) they ship in 2.0.x |
