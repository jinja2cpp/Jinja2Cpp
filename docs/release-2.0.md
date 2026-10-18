# Jinja2C++ 2.0: release gates and RC signoff

This is the one place that says what 2.0 ships, what blocks the tag, what is accepted as
a known limitation, and how a release candidate (RC) is approved (docs/tasks/0221). The
API itself is designed in [api-2.0.md](api-2.0.md); individual fixes stay in their task
files, and this document links to them rather than repeating them.

A green parity suite means "no difference outside the recorded divergences". It is not
a claim of complete Jinja2 compatibility, nor of safety for untrusted templates. Both
claims the release does make are bounded in sections 5 and 3 (G3) below.

## 1. Status

| | |
|---|---|
| Release manager and approver | Ruslan (`rmorozov`) |
| Version | 2.0.0, SOVERSION 2 (already set: `CMakeLists.txt`, `project(... VERSION 2.0.0)` and `SOVERSION 2`) |
| Freeze | not yet; the API freeze waits on G1 and the decisions in section 8 |
| Candidate commit | **none yet**. The RC is recorded here as a full SHA when it is cut (section 7) |
| Reference commit for this revision | master `345d0c6` (2026-10-18) |
| Last observed CI on master | `e69c8ad` (#561): linux, macOS, Windows, Conan, CodeQL and benchmark workflows all passed; `345d0c6` changed only `docs/` and ran the format check alone |

"Configured" below means a CI job exists; "observed" means a passing run on a named
commit. Only observed runs on the candidate commit count for signoff.

## 2. Scope of 2.0

2.0 ships what master has plus the blocking tasks of section 3:

- The 2.0 public API of api-2.0.md: `Value` accessors, `TemplateEnv`/`Settings` renames,
  containers and reflection, ordered `ValuesMap`, `Result<T>` on pinned expected-lite,
  `[[nodiscard]]` accessors, unknown filters/tests as Load errors, v1 names kept as
  `[[deprecated]]` one-line forwards (api-2.0.md 5.1).
- C++17 as the language floor; C++17, 20 and 23 supported and tested.
- The template language as measured by the parity corpus (section 5) against the oracle
  pair named in section 8 (D7).
- Inline ABI namespace `jinja2::v2`, `fwd.h`/`jinja2cpp.h` headers (0076).

Not in scope, and not release conditions: the rest of 0117 and every other performance
follow-up, 100% of a corpus that keeps growing, a sandbox, configurable resource limits,
big integers, a Boost-free build. Each of these keeps its own task.

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
| G1 API/ABI freeze | Inline namespace `v2` in every public header, `fwd.h`, no leaked macros or vendored names; `nm` shows `jinja2::v2::`; a consumer built against 1.3.2 headers fails to link against 2.0 (static and shared); decisions D1-D6 recorded | [0076](tasks/0076-abi-namespace-and-headers.md) | 0076 thread (running) | B | 0076's tests in CI; negative link check |
| G2 Known correctness defects | `*context` callable binds positional args; self-referencing namespace/dict/list does not leak under LSan; unsigned above `INT64_MAX` renders the value or fails loudly | [0183](tasks/0183-context-callable-positional-arg.md), [0110](tasks/0110-namespace-self-reference-leak.md), [0172](tasks/0172-value-unsigned-overflow.md) | one thread each (started 2026-10-18) | B | their unit rows; sanitizer jobs with `detect_leaks=1` |
| G3 Embedding and trust contract | Lifetime/cache/callback scenarios pass under ASan/UBSan/LSan and TSan; `docs/security.md` states the trust model, what `RealFileSystem` does with `../` and absolute names, the fixed caps, and a reporting contact | [0223](tasks/0223-embedding-lifecycle-integration-tests.md), [0224](tasks/0224-template-trust-and-loader-boundary.md) | unassigned | B | sanitizer jobs; 0224's disposable-file probes |
| G4 Migration | `MIGRATION.md` covers renames and behaviour changes with before/after; the migration script rewrites our 1.x-style tests warning-free in CI; a 1.3.2 consumer fixture upgrades against the installed RC | [0077](tasks/0077-migration-script.md), [0222](tasks/0222-downstream-1x-migration-trial.md) | unassigned (after G1) | B | CI job of 0077 and 0222 |
| G5 Distribution | License conclusion recorded and LICENSE/notices installed; the source archive builds with tests off in a clean directory and a separate consumer uses it via `find_package` and pkg-config; Conan recipe validated with an installed-package consumer | [0226](tasks/0226-license-metadata-and-distribution.md), [0225](tasks/0225-release-artifact-consumer-gate.md), [0006](tasks/0006-conan-ci-and-releases.md) | unassigned | B (ConanCenter publication: A, D8) | 0225's check output with archive hash; Conan consumer job |
| G6 Docs and examples | README, `MIGRATION.md` and the versioned 2.0 site agree on C++17, CMake ≥ 3.23 and dependencies; every advertised snippet and example builds against the RC; historical repositories labeled | [0227](tasks/0227-supported-docs-and-example-surface.md) (+ README via 0077) | unassigned | B | `docs/project-support.md` validation table |
| G7 Parity | The corpus passes on the candidate with no `crash` and no `unsupported` entries; every remaining divergence line names an open task or is `deliberate:`; the accepted list (section 5) is current | — | merge steward | B | `ctest -R parity`; `parity-expectations` workflow |
| G8 Robustness | Full CI green incl. sanitizer jobs; the nightly fuzz workflow passes on the candidate (dispatch it); `fuzz/regressions/` replays clean | — | merge steward | B | observed runs on the candidate |
| G9 Performance | The benchmark workflow passes against the previous master (no regression past its gate). No further gains required | — | perf track | B | `benchmark.yml` run on the candidate |
| G10 Inventory | `scripts/sbom.py --check` and `scripts/task_index.py --check` pass; the dependency inventory matches what the release installs | [0187](tasks/0187-generated-data-acquisition.md) (SBOM stays there) | merge steward | B | format-check job |

Dependency order: G1 → G4 (0077 → 0222); 0226 → 0225; 0227 alongside 0077. G2, G3 and
G5's 0226 can start now.

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
- **Dependencies in the interface.** `Result<T>` stays `nonstd::expected` (pinned) for
  all of 2.x; moving to `std::expected` needs a C++23 floor and is a 3.0 question.

## 5. Accepted limitations

Corpus on master `345d0c6`: 1929 cases, 1886 match, 43 differ (25 output, 14 rejects,
4 accepts, 0 crash, 0 unsupported); `docs/parity.md` has the per-area table.

**Deliberate (14 lines), documented, not bugs:**
- 64-bit integers: overflow raises, a literal beyond int64 becomes a float (0015, 4 lines;
  big integers are 0178).
- Names captured by a closure that outlives its frame read as undefined (0165, 6 lines).
- `select`/`map`/`reject` results can be iterated twice and test eagerly (0101, 2 lines).
- `%c` of a lone surrogate in wide templates (0106); a call block returning a dict
  prints it (0156).

**Open, low impact, accepted for 2.0 (30 lines):** 0214, 0219 (call protocol and loop
object), 0213, 0049, 0220, 0211, 0215, 0178 (2 non-deliberate), 0036, 0203, 0209, 0210,
0212, 0216, 0041. They keep their tasks and may land in 2.x minors. 0216 (volatile
`{% autoescape %}`) is in this list on the recommendation of D10.

**Outside the corpus:**
- Fixed resource caps only: `MaxSequenceSize` (2^26 items/characters) and the compile-time
  recursion limits of `src/recursion_guard.h`. No configurable time/memory budget (0097,
  0098); 0224 documents the containment a host must provide.
- No Markup flag on host values (0180), no per-template-name autoescape (0181).
- `Template::Load`/`Render` are not `[[nodiscard]]` (0080, D6).
- Python-version-dependent behaviour (Unicode tables, Jinja2 3.1.6 quirks) follows the
  pinned oracle pair (D7).

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

## 7. RC procedure

1. **Freeze.** When G1-G6 are done and section 8 is decided, the steward picks a master
   commit, records its full SHA in section 1 and opens the validation window. From then
   on master takes only fixes for blocking findings; other merges wait.
2. **Validate (window: D11).** On the candidate commit, observe and record here, with run
   links: every build workflow, sanitizers, `parity-expectations`, a dispatched `fuzz` run,
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
   to section 5; nothing is waived silently or by an agent.
5. **Signoff.** Ruslan approves the candidate SHA; then: tag `v2.0.0` on that SHA, publish
   the source archive whose hash 0225 recorded, the release notes (`MIGRATION.md` plus
   section 5), the versioned docs site, and the Conan recipe update (0006). The tag commit
   must equal the validated SHA.

## 8. Decisions for Ruslan

Each has a recommendation; the release can proceed on the recommended answer unless
Ruslan picks another. Answers are recorded here and in the linked task.

| # | Question | Options | Recommended | Why |
|---|---|---|---|---|
| D1 | `Value(uint64_t)` above `INT64_MAX` (0172) | a) throw from `Value`/`Reflect`; b) store as `double`; c) a `uint64_t` alternative | **a** | Loud and cheap; leaves room for 0178 to later turn the throw into a big integer without a source break. b loses precision silently, c costs every evaluator path |
| D2 | Big integers in 2.0 (0178) | a) defer past 2.0; b) in 2.0 | **a** | Value-model work needing an architect plan; 0015's int64 behaviour is documented as deliberate |
| D3 | 0178 (Boost.Multiprecision `cpp_int`) against 0230 (Boost-free core) | a) both stand: `cpp_int` in standalone mode (`BOOST_MP_STANDALONE`, header-only, Config only) as a private, non-installed dependency, and 0230's Done reads "no Boost libraries and no Boost in the public interface or installed package"; b) an internal big-integer type; c) drop big integers | **a** | The contradiction is about linked and installed Boost, which standalone header-only use in `src/` does not bring back; b is a correctness-critical reimplementation; c gives up parity for packaging. Neither task blocks 2.0 |
| D4 | Markup on host values (0180) | a) defer, later as an additive change with a SOVERSION bump; b) in 2.0 | **a** | A new `Value` alternative breaks users' exhaustive visitors; better designed once than rushed into the freeze |
| D5 | When deprecated v1 names are removed | a) kept for all of 2.x, removed in 3.0; b) removed in a 2.x minor | **a** | Matches the source promise of section 4 |
| D6 | `[[nodiscard]]` on `Template::Load`/`Render` (0080) | a) in 2.0; b) later | **a** | Adding it later is a new warning, an error under `-Werror`; 2.0 already breaks source, so this is the cheap moment |
| D7 | Oracle pair (0186) | a) one pinned pair per release (now Python 3.12 + Jinja2 3.1.6), named in `docs/parity.md` and `version.h`; b) selectable targets via CMake | **a** for 2.0 | Selectable table sets are new build surface; one documented pair is enough for the release, b stays open in 0186 |
| D8 | Conan publication as a gate (0006) | a) RC needs a validated recipe and an installed-package consumer; ConanCenter publication follows the tag; b) tag waits for publication | **a** | Publication depends on ConanCenter reviewers, outside our control |
| D9 | Supported platforms | a) the table in section 2; b) add or drop rows | **a** | It is what CI observes |
| D10 | Volatile `{% autoescape %}` (0216) | a) accepted limitation; b) blocks | **a** | One corpus case, needs a runtime-valued `{% autoescape %}` around a macro; documented in section 5 |
| D11 | RC validation window | a) 7 days with no blocking finding; b) 14 days | **a** | Single maintainer; G4/G5 trials already exercise downstream use |
| D12 | Resource and recursion limits (0097, 0098) and `select_autoescape` (0181) | a) defer; additive `Settings` fields in 2.x with SOVERSION bumps; b) in 2.0 | **a** | Fixed caps exist; 0224 documents host-side containment. The SOVERSION policy is made for exactly these additions |
| D13 | Non-string mapping keys in the public API (0036) | a) public `ValuesMap` keys stay `std::string` for 2.x; non-string keys only inside templates if 0036 lands; b) decide the public key type now | **a** | Changing the public key type later is a source break; keeping strings is the stable promise |
| D14 | 1.3.2 consumer for 0222 | a) a self-contained fixture in `test/migration/`; b) a real downstream project | **a** | Needs no owner's consent and runs in CI; b can follow |
