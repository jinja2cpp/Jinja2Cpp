# Jinja2C++ code review guidelines

Apply these criteria to automatic reviews, requested PR reviews and pre-push
verification. Read [AGENTS.md](AGENTS.md) for the shared project workflow. Review
the diff against its actual target branch and read enough surrounding code/tests
to establish its behavior; a design document or green check alone is not evidence.

## Findings and scope

- Report actionable defects introduced or materially exposed by the proposed change.
  State the concrete input/call sequence, consequence and affected configuration.
  A known backlog item is context, not a finding on every PR. Do not request unrelated
  roadmap work simply because it is unfinished.
- Prioritize memory safety, escaping, silent wrong results, deadlocks, build/package
  failures and unintended API/ABI breaks. Follow with material performance regressions
  and missing regression coverage tied to an identified failure mode.
- Check the claim before reporting it: use a minimal reproducer, the pinned Python
  oracle, a relevant existing test, or a precise code path. Clearly distinguish a
  reproduced defect from a reasoned finding. If essential assumptions cannot be
  checked, ask a focused question instead of presenting speculation as a bug.
- Do not flag formatting preferences, routine refactoring opportunities or hypothetical
  failures contradicted by an invariant or test. Do not demand a new test that merely
  repeats the implementation or an unrelated platform matrix for a documentation edit.
- Review without changing tracked files, regenerating fixtures, committing fixes or
  posting extra follow-up comments unless explicitly requested. Scratch probes may
  use disposable files and separate build directories. Never access host-sensitive
  files or external services to demonstrate a loader or callback issue.

## Behavioral parity

- Use the Python and dependency versions pinned by the parity workflow and
  `test/parity/requirements.txt`. Compare output and rejection behavior, including
  undefined versus None, truthiness, argument binding, whitespace and Unicode.
- Check narrow and wide paths. Passing a narrow test does not establish TemplateW
  behavior, especially for encoding, character classification and output conversion.
- Check nearby invalid inputs: wrong arity/types, duplicate or unknown keywords,
  empty containers, invalid UTF-8 where relevant, and malformed template syntax.
  Errors must propagate through the documented API rather than crash or disappear.
- Read relevant divergence entries and task decisions. An intentional difference
  is not a regression, but a newly required allowance needs justification. Green
  parity tests mean outputs agree with expectations/allowances, not complete parity.
- Verify case changes regenerate their Python expectations; fixed differences should
  leave the allowlist. Do not approve weakened assertions or skipped reproducers as
  a substitute for a fix. Keep generated summary updates in the integration workflow.

## Ownership, lifetimes and concurrency

- Trace the owner of borrowed strings, views, reflected containers, generator items,
  returned callable data, arena nodes and stored macro/self/super callables. Check
  their use after the defining call/frame has ended and frame/arena storage is reused.
- Consider failed reloads, cache eviction, includes/imports, inherited templates and
  templates that outlive their environment. Verify cleanup on errors and exception
  paths as well as successful rendering; inspect reference cycles for leaks.
- Distinguish host-owned input from template-created mutable values. Check documented
  aliasing/copy behavior, iterator invalidation and mutation during traversal. Do not
  assume Python identity/mutation rules are implemented without checking decisions.
- Review concurrent renders, thread-local workspace reuse, nested/reentrant renders
  and callbacks into the same environment. Look for shared mutable state, lock-order
  problems, callbacks invoked under locks and partial publication of cached objects.
- Respect supported concurrency boundaries: settings and handler configuration are
  not synchronized for concurrent mutation/loading/rendering. Do not report misuse
  outside that contract as an introduced race. Check applicable supported operations
  with TSan when available, and report checks that could not be executed.

## Escaping and trust boundaries

- Follow autoescape and Markup through values, macros, caller blocks, filters,
  concatenation, imports and C++ callable conversions. Distinguish lexical escaping
  from runtime evaluation context; look for unescaped output and double escaping.
- Treat Python compatibility and security claims separately. A recorded divergence
  does not establish that HTML is safe. Conversely, explicit safe/Markup input is
  trusted by design; do not report that behavior alone as an escaping vulnerability.
- Check arithmetic overflow, allocation/sequence sizing, recursion, output growth and
  termination for changed operations. Do not equate a fixed allocation cap with a
  complete per-render budget or claim a C++ callback can be forcibly interrupted.
- When loaders change, inspect relative/absolute paths, traversal, symlinks and
  platform-specific path rules against the documented policy. Do not assume that
  a root-folder parameter provides confinement or that a full sandbox exists.

## Public API, ABI and distribution

- Keep public headers C++17-compatible and the library/consumer vocabulary types
  consistent across standards. Check Result/expected selection, namespace/export
  macros, public layouts, signatures and static/shared definitions. A CMake version
  or SOVERSION alone does not prove ABI isolation.
- Compare API changes with `docs/api-2.0.md`, implemented behavior and the task's
  accepted decisions. Separate intentional major-version breaks from accidental
  ones, and require migration guidance for intentional source or template changes.
  Do not assume every proposal in that design document has already landed.
- For install/dependency changes, inspect exported targets and transitive include,
  compile and link requirements. Consider tests-disabled builds, external/Conan
  dependencies, JSON binding choices and a separate installed-package consumer.
  Reuse `test/package/check_package.cmake` when it covers the affected path.
- Distinguish building with Conan-resolved dependencies from creating or publishing
  a consumable Jinja2Cpp recipe. Flag concrete mismatches in supported dependency
  pins, hashes, license metadata, notices or release contents affected by the change.
- Inspect platform-sensitive code even when it cannot run locally: MSVC shared-build
  exports, internal symbols used by tests, template instantiation/capture differences,
  Apple Clang warnings and locale-dependent wide-string expectations. State the
  limitation instead of claiming cross-platform verification.

## Performance and validation evidence

- For a performance claim, compare matching workloads/configurations and verify
  rendered output first. Use the existing benchmark/gate tooling, accounting for
  noisy wall time, cold versus warm caches, Load versus Render and allocation costs.
  Do not infer a measured speedup from a plausible optimization or old benchmark table.
- For a suspected regression, identify the affected workload and evidence rather
  than demanding optimization everywhere. Preserve stated performance budgets and
  check that work has not simply moved outside the timed region.
- Run checks appropriate to the diff, using available tooling and isolated outputs.
  Distinguish actual test execution/counts from a configured job, zero-test run,
  skipped case or cached artifact. Do not claim an unobserved CI result.

## Documentation, task and instruction PRs

- Check that snippets, links, versions, tool requirements and claimed behavior agree
  with the source and supported workflow. Instructions must not treat runner-specific
  hooks, credentials, models or network restrictions as universal facts.
- New tasks need a unique number, correct existing references/dependencies, scoped
  touches/shares, a concrete problem and verifiable completion criteria. Check for
  duplication and distinguish a confirmed bug from a report awaiting reproduction.
- The merge steward owns the generated task index. Expected task-only index drift
  is not a defect. Link changing release priorities to the current task registry;
  do not maintain a second hard-coded release backlog in this review guide.
- For agent-instruction changes, check that referenced files exist, common guidance
  has a clear home, and review instructions do not silently authorize edits or merges.
  Instruction files guide review behavior; they do not configure automatic triggers.

## Report format

Use the review tool's required output format. For each finding, provide a short
imperative title with severity, the smallest useful changed-line location, and a
concise explanation of the trigger and consequence. Explain configuration or usage
assumptions and cite the relevant contract/test where helpful. Suggest the direction
of a fix without writing an unsolicited patch or duplicating the same root cause.

Priorities describe the finding's impact, not its existing backlog label:

- P0: unconditional critical failure requiring immediate intervention.
- P1: serious correctness, safety, compatibility or availability defect to fix promptly.
- P2: material defect in a supported, identifiable scenario, normally scheduled work.
- P3: minor improvement; omit cosmetic findings unless that scope was requested.

Order findings by severity. Report actionable P0/P1/P2 defects with evidence rather
than filling a quota. If no actionable issue is found, say so when the output format
allows, and identify material verification limits. A clean review is not a guarantee
of correctness. Reserve a ready/blocking verdict for a requested pre-push verification
or other workflow that asks for one; an automatic review should not merge the PR.
