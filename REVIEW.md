# Jinja2C++ code review guidelines

These criteria are shared by every reviewer of this repository: Codex, Claude Code
Review and the pre-push verifier. They state what counts as a defect here; each tool
keeps its own output format (severity mapping at the end). Read [AGENTS.md](AGENTS.md)
for the project workflow. Review the diff against its actual target branch and read
enough surrounding code and tests to establish its behavior; a design document or a
green check alone is not evidence.

Write a finding only for what the change introduces or materially exposes. Generic
advice that any reviewer would give is not the point of this file: the rules below
are what this codebase has actually got wrong.

## Evidence and scope

- State the concrete input or call sequence, the consequence and the affected
  configuration. Check the claim first: a minimal reproducer, the pinned Python
  oracle, an existing test, or a precise code path. Say whether a finding was
  reproduced or reasoned. If an essential assumption cannot be checked, ask a focused
  question where your output format allows one; otherwise omit the claim rather than
  presenting speculation as a bug.
- A known backlog item (`docs/tasks/`) is context, not a finding on every PR. A new
  TODO or known gap the change leaves behind without a task file is a finding.
- Review without changing tracked files, regenerating fixtures, committing fixes or
  posting extra comments unless asked. Scratch probes use disposable files and
  separate build directories. Never touch host-sensitive files or external services
  to demonstrate a loader or callback issue.

## Security: template text and data may be untrusted

- **No crash on any input.** Malformed templates, deep nesting and huge literals end
  in an error `Result`, never an abort, stack overflow or uncaught exception. New
  recursion in the parser, evaluator or value methods (including cycle checks) goes
  through `src/recursion_guard.h` and reports `RecursionLimitExceeded`.
- **Template-controlled sizes are bounded.** A sequence or string whose size comes
  from template text or data (`range`, `*` repetition, center/indent/format widths,
  join over generators) goes through the existing hard cap (`MaxSequenceSize` and
  its check in `src/internal_value.h`), with the size arithmetic checked before the
  allocation and integer conversions following task 0172. Configurable limits are
  still open (task 0097): a missing per-render budget is not a finding, but a new
  path that bypasses the existing cap is.
- **Errors reach the caller as a `Result`.** Load, Render, `GetMetadata*` and
  `TemplateEnv::LoadTemplate*` return every failure in `Result`; only `std::bad_alloc`
  escapes, meaning the system is out of memory (docs/api-2.0.md 2.1, task 0253). The
  boundary is `detail::CurrentExceptionToError` (`src/error_boundary.h`): a host
  `jinja2::Exception` keeps its code, a host callable's exception becomes
  `UserCallableException`, any other exception becomes `UnexpectedException`,
  and `GetException()` holds the original only when the exception came from outside the
  engine. An engine limit throws `ResourceLimitError` (`src/engine_errors.h`, code 18,
  no exception kept), not a bare `std::length_error`. Flag a catch that swallows
  `std::bad_alloc` (a `catch (std::exception&)` or `catch (...)` that does not rethrow it),
  a `noexcept` function or destructor that can allocate or throw, a value returned into
  expected-lite's `noexcept` converting constructor without a local copy, and code that
  leaves an arena, table or template unusable when an allocation fails
  (`jinja2cpp_alloc_tests` exercises Load and Render with each allocation failing).
  Engine errors still reported as standard exceptions get their own codes in 0255.
- **Render errors are located (task 0254).** Each statement body (`ComposedRenderer`)
  turns what a statement throws into `detail::RenderFailure`, located at that statement's
  tag; bodies around it pass it on unchanged. So no exception may be control flow across
  a statement body: a `catch` that expects an engine error from inside a rendered body
  sees a `RenderFailure` instead. Flag such a catch, and a body made without its tag
  offsets (`StatementBody::Take`). What a host callable throws becomes
  `detail::HostCallableFailure` (code 15) in `UserCallableBody`; the engine's own error
  types pass through it.
- **Lifetimes.** The worst historical class (0112, 0115, 0125, 0134). Trace the owner
  of every `string_view`, lent item, generator item, reflected nested item, container
  returned by a callable, arena node (`NodeRef`) and stored macro/self/super callable.
  Flag use after the statement, frame, render or reload that owned it, and escaping
  callables or lambdas holding raw `this` instead of a keep-alive. Check error and
  exception paths, failed reloads, cache eviction, includes/imports, inheritance and
  templates outliving their environment. Check mutation of a container during its
  own traversal and iterator invalidation from callbacks.
- **Loaders.** The loader trust boundary is still being settled (task 0224). Do not
  assume a root folder confines template names, and flag a change that makes name
  handling more permissive or adds file access outside the FileSystem handlers. Once
  0224 lands, its policy is the rule.
- **Autoescape.** Follow Markup through values, macros, caller blocks, filters,
  concatenation, imports and C++ callable conversions. Markup + str escapes the str;
  `tojson` escapes `<`, `>`, `&` and `'` like Python; double escaping is a bug too.
  Explicit `safe`/Markup input is trusted by design and is not a vulnerability.
- **Format strings.** Template text never becomes a fmt format string except through
  `src/python_format.cpp`, with its exceptions mapped to errors.
- **Concurrency.** One Template renders on many threads, and a TemplateEnv cache is
  shared. No mutable static or thread-local state may leak between renders (a reused
  render workspace is fully reset); callbacks may re-enter the same environment; no
  callback runs under a lock; cached objects are published only when complete.
  Settings and handler mutation concurrent with rendering is outside the contract.
  Check supported operations with TSan where available.
- **Supply chain.** A new or bumped dependency is pinned with its `URL_HASH` in
  `thirdparty/internal_deps.cmake`, listed in `sbom/components.json` and has a
  compatible license. TLS and checksum verification are never weakened.

## Behavioral parity

- Use the Python and package versions pinned by `.github/workflows/parity-expectations.yml`
  and `test/parity/requirements.txt`. Compare output and rejection: undefined versus
  None, truthiness, argument binding, whitespace control, Unicode.
- Check narrow and wide paths: a passing narrow test does not establish TemplateW
  behavior, especially for encoding, character classification and output conversion.
- Check nearby invalid inputs: wrong arity or types, duplicate or unknown keywords,
  empty containers, invalid UTF-8, malformed syntax.
- An entry in `test/parity/divergences/<area>.txt` is an accepted difference, not a
  regression; a newly added entry needs a justification. A fixed difference leaves
  the allow-list. Weakened assertions or skipped reproducers are not a fix.

## Performance

- **Measured claims need counts.** A claimed speedup, or a claimed regression of a
  stated size, needs `bench/count.py` instruction counts on named workloads. Cloud
  wall time is noise. Do not suggest optimisations outside the hot-path list below
  without numbers.
- **The bar is tighter than preflight.** Preflight fails at +3%, but a performance PR
  merges only on the performance track's verdict, read from CI's "instruction counts
  vs base" job: a regression above +0.5% on any Load or Render benchmark is a named
  exception for the merge steward, while the intended improvements are the point. A
  +2% Load regression is a finding. The job does not gate I1 cache misses (mostly
  layout noise), but limits a phase plan sets on them still apply
  (`docs/design/perf-design-overview.md`, `docs/design/0038-lexical-scoping-plan.md`). That
  job compares against the PR's base at its last push, so on a stale base the delta
  may come from master; compare against current master (as the steward's train run
  does) before calling it the PR's regression.
- **Hot path budget.** These constructs are findings by inspection alone, without
  counts, because each has cost measurable time before. On the per-node render and
  evaluation path, flag a new heap
  allocation, a copy of `std::string` or `InternalValue` where a lent value would do,
  a new virtual call or `shared_ptr` refcount per element, an exception used for
  control flow, or a by-name lookup where a name slot exists (0117).
- **Footprint.** `InternalValue` is 32 bytes (`static_assert` in
  `src/internal_value.h`, task 0140); growing it, or another hot type, needs a reason.
- **Complexity.** No quadratic string building or repeated rescans of input. Lookup
  invalidation stays bucketed per name (0158).
- **Load versus Render.** Moving work into Load is fine when it amortises, but Load
  regressions count: one-shot templates pay them on every use. Check that work has not
  simply moved outside the timed region.
- **Phased work.** A temporary regression is accepted when the PR names the later
  phase of its task that removes it.

## Maintainability, public API and distribution

- **Ownership is explicit.** One owner per object; access through typed handles
  (`NodeRef`) or `boost::span`-style views. A new raw owning or long-lived raw pointer
  needs a stated reason; non-owning observers say so.
- **Public API and ABI.** Public headers stay C++17 with consistent vocabulary types
  (`Result` uses the pinned expected-lite). New exported functions carry the export
  macro (MSVC shared builds). Renamed API keeps a `JINJA2CPP_DEPRECATED` forward. A
  change to public layouts, signatures or namespaces needs the task decision that
  allows it (`docs/api-2.0.md`, task 0076) and migration notes for intentional breaks.
  A version or SOVERSION bump alone does not prove ABI isolation.
- **Packaging.** For install or dependency changes, check exported targets and
  transitive include/compile/link requirements, tests-disabled builds, Conan and
  external dependency modes, JSON binding choices and a separate installed consumer
  (`test/package/check_package.cmake`).
- **Platforms not built in the cloud.** Inspect MSVC exports and internal symbols used
  by tests, lambda captures and eager template instantiation, Apple Clang warnings
  (unused private fields) and locale-dependent wide-string expectations. State the
  limitation instead of claiming cross-platform verification.
- **Reuse before writing.** Prefer existing helpers (`recursion_guard`,
  `CompareForOrder`, `python_format`, value conversions and lending) over a second
  copy; flag narrow/wide duplication that one template would remove.
- **Errors are typed.** New failures use an `ErrorCode` with location and parameters,
  not ad-hoc strings.
- **Hygiene.** No new warning suppression, `NOLINT` or `#pragma` without a reason;
  `if (p)`, not `p != nullptr`; comments explain why and stale comments go.
- **Tests.** A behavior change has a test row in an existing table or a parity case,
  covering the error path and both character types. Tests do not depend on timing,
  sleeps or the C library locale, and check the failure mode rather than restating
  the implementation.

## Documentation, task and instruction PRs

- Snippets, links, versions and tool requirements agree with the source and CI.
  Instructions do not present runner-specific hooks, credentials, models or network
  restrictions as universal facts.
- New tasks have a unique number (check master and open PRs), correct references,
  scoped touches, a concrete problem and verifiable completion criteria. Task-only
  drift of the generated index in `docs/tasks/README.md` is expected; the merge
  steward regenerates it.
- Agent-instruction changes: referenced files exist, shared guidance has one home,
  and review instructions do not authorize edits or merges.

## Do not flag

- Formatting that `.clang-format` or the changed-line format check owns.
- Unfinished roadmap work unrelated to the diff.
- Differences already on a divergence allow-list.
- Micro-optimisations without instruction counts, other than the hot-path list.
- Requests for more comments, or tests for pure refactors already covered.
- Platform matrices for documentation-only changes.
- Hypothetical failures contradicted by an invariant, a check or a test.

## Severity

Severity describes impact, not the backlog label. Order findings by severity and do
not fill a quota. Where your output format allows a summary, say when nothing
actionable was found and name what could not be verified. A clean review is not a
guarantee, and no automatic review merges a PR.

| Impact | Codex | Claude Code Review |
|---|---|---|
| Memory corruption (use-after-free, overflow) or an autoescape bypass reachable from template text or data with default settings | P0 | red (blocking) |
| Crash, abort or unbounded allocation reachable from template text; silent wrong output in common use; build or package break on a supported configuration | P1 | red (blocking) |
| Material defect in a supported, identifiable scenario | P2 | red when the scenario is supported, else yellow |
| Minor improvement | P3 | yellow (nit) |
