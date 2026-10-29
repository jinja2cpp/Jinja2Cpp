# Clang lifetime, thread-safety and other analyses next to the TLA+ model and the sanitizers

Ruslan asked (2026-10-29): would Clang's
[Lifetime Safety](https://clang.llvm.org/docs/LifetimeSafety.html) and
[Thread Safety Analysis](https://clang.llvm.org/docs/ThreadSafetyAnalysis.html) help our
borrow-checking work on top of the TLA+ model and the sanitized runs, or do those already cover the
same ground? And are there other Clang tools we are missing? This note gives the answer, the
evidence from prototypes, and the follow-up tasks (0343-0345).

## Short answer

They cover different ground. Each of our three checks has a blind spot that one of the others
covers, so the Clang analyses are worth adding where they are cheap. Most of the value comes from
the stable annotations (`[[clang::lifetimebound]]`, `[[clang::lifetime_capture_by]]`,
`GUARDED_BY`/`REQUIRES`). Clang's new flow-sensitive Lifetime Safety analysis is still
experimental. It is worth running as a non-blocking report, but it is not worth a gate yet.

| Technique | What it checks | Paths covered | Blind spot |
|---|---|---|---|
| TLA+ model (`tla/`) | the design of the cell protocol: every interleaving of every step, up to a bound | all, within the model's bound | the C++ code itself; memory order (it assumes sequential consistency); anything not modelled |
| Trace validation (#691) | that a C++ run is a behaviour of the model | the runs we execute | code paths we never run |
| ASan / UBSan / TSan | the real code on real memory, including memory order (TSan) | only the runs we execute | a path no test reaches; a race whose timing never occurs |
| Clang annotations (lifetimebound, capture_by, thread safety) | local rules written on declarations, checked at every call site | all, at compile time | anything not annotated; anything across functions; destructors (thread safety); dynamic ownership |
| Clang Lifetime Safety (flow-sensitive, experimental) | loans and their expiry inside one function, without annotations | all paths of one function | calls into unannotated functions are trusted; experimental |

An analogy. Think of a bridge. The TLA+ model is the engineer's calculation: it proves the
design holds under every combination of loads, but it says nothing about whether the builders
used the right bolts. Sanitizers are load tests on the built bridge: they find real defects, but
only under the trucks we actually drive across it. Annotations are the inspector who checks the
stamp on every bolt as it goes in: every bolt, at build time, but only against the rule that is
stamped on it, and never how bolts interact across the whole span.

## The contradictions each tool resolves

- **Model vs code.** The model proves the design but runs no C++. Trace validation and the
  sanitizers bring that back to the code, but only for runs we execute. Annotations close part of
  the gap from the other side. They are static and cover all paths, so a rule the model relies on
  ("a view never outlives the promoter", "this map is read only under `guard`") is checked on every
  call site, not just on the tested ones.
- **All paths vs real semantics.** Static checks see every path, but only the local property that
  an annotation states. The sanitizers see the real, global behaviour, but only on executed paths.
  Neither one replaces the other.
- **The next contradiction.** An annotation can be wrong, and a wrong annotation gives false
  confidence. The sanitizers and the model stay the referee: a `lifetimebound` that does not hold
  shows up as an ASan report on a covering test, and a `GUARDED_BY` on the wrong mutex shows up as
  TSan noise. So the three checks should be kept together, not traded for one another.

## Past bugs, technique by technique

| Bug | Kind | Found by | Clang annotations | Clang Lifetime Safety (22.1.8) | TLA+ |
|---|---|---|---|---|---|
| 0296: a typed `string_view` param views a temporary promoter's member | dangling view, one call | ASan (stack-use-after-return) | **yes**: two annotations make Clang 20 flag the exact line (below) | no: it trusts the unannotated conversion operator | out of scope (C++ type level) |
| 0337: a nested module call shares the outer call's export mark | protocol logic, one thread | TLA+ (F1) | no: memory stays valid, the value is wrong | no | **yes** |
| 0339: relaxed `use_count()==1` used as an "alone" test | memory order on an atomic | TSan (with an acquire-load experiment) | no: thread safety analysis does not reason about atomics | no | no: the model assumes SC |
| 0340: a host thread drops the last ref of an adapter linked to a live cell | which thread runs a destructor | TSan | no: destructors are not checked, and "last ref" is dynamic (prototype below) | no | yes, once modelled (0338 step A, `FixPinOnAttach`) |

So among these four, the annotations would have caught 0296 at compile time, before any test
existed. They would not have caught the protocol and memory-order bugs, which is where the model
and TSan do the work. 0296 is the commonest kind of bug in this project, though: REVIEW.md names
lifetimes as the worst class of defect (0112, 0115, 0125, 0134, 0296). These bugs live in the
host-API glue (`user_callable.h`, reflection, value views), and a compile-time check fits that
code well.

## Prototype 1: lifetimebound + lifetime_capture_by on 0296

The files are in the project folder, `clang-analysis/`. The 0296 bug on the pre-fix header
(00899ad3) is `param->SetValue(Promote(v))`: the temporary promoter's
`operator other_string_view()` returns a view of its own `m_convertedStr`, and `SetValue` stores
that view in the param. We added two annotations:

```cpp
operator other_string_view() const [[clang::lifetimebound]]                  // result views *this
void SetValue(decayed_t val [[clang::lifetime_capture_by(this)]])            // val is stored in *this
```

- Clang 20 (`-Wdangling-capture`, on by default), compiling a host file that calls
  `MakeCallable(&Len, ArgInfo{"s"})` with `Len(std::string_view)`, warns at
  `user_callable.h:353`: "object whose reference is captured by 'this->param' will be destroyed at
  the end of the full-expression". It warns for both the `std::wstring` and the `std::wstring_view`
  instantiations. That is the bug, found at compile time with no test.
- Clang 18 knows `lifetimebound` but not `lifetime_capture_by`. It catches only the local form
  (`std::string_view v = Promote(w);`, `-Wdangling`).
- Without the annotations, no Clang version warns. Clang 22.1.8's experimental Lifetime Safety
  analysis does not warn either, in permissive, strict or inference mode.
- Applied to current master, the same two annotations give one class of false positive. The
  generic `param->SetValue(Promote(v))` after the 0296 `if constexpr (...) { ...; return; }` is
  still instantiated for the other-width view cases, where it is dead code. With an `else`, the
  warning goes away: zero warnings on `test/user_callable_test.cpp`. A `decayed_t&&` parameter
  instead of a by-value one warns on every owning type (`std::string`, `int`). The annotation
  belongs on a view-typed value parameter.

Cost: the attributes must be hidden behind macros (`__has_cpp_attribute`). GCC warns on unknown
scoped attributes and we build with `-Werror`, Clang 18 lacks `lifetime_capture_by`, and MSVC
spells it `[[msvc::lifetimebound]]`. The public headers stay C++17.

## Prototype 2: Thread Safety Analysis

**On real locks.** We replaced `std::shared_timed_mutex guard` and `std::mutex m_loadSettingsGuard`
in `TemplateEnvImpl` with thin annotated wrappers (libstdc++ does not annotate its mutexes), and
turned the existing comments into checks: "Guarded by `guard`" became
`globalValues GUARDED_BY(guard)`, `m_loadSettings GUARDED_BY(m_loadSettingsGuard)`, and
`BigIntBackendLocked() REQUIRES(m_loadSettingsGuard)`. The lock sites in `template_env.cpp`,
`template_impl.h` and `template_module.cpp` became the annotated scoped locks. The patch is
`clang-analysis/tsa-template-env-prototype.patch`. Result with
`-Wthread-safety -Wthread-safety-beta` on Clang 20: every access already holds its lock. There is
one false positive: `IsEqual` takes two `std::shared_lock`s with `std::defer_lock` + `std::lock`,
which the analysis cannot follow. This is the usual cost, and the usual fix is an annotated
two-lock helper. So the analysis found no bug, but it turns three comments into compile-time rules
that every later edit has to keep.

**On the cell protocol (0340).** The cell protocol has no mutex. Its rule is thread confinement:
"a live cell is touched only on its render's thread". Thread Safety Analysis can model that as a
role capability (`capability("role")`, `ASSERT_CAPABILITY`). The reduced prototype
`clang-analysis/tsa0340.cpp` shows the limit. A direct call of `Unlink()` from a host function is
flagged. The real 0340 path, `delete` → `~Link()` → `Unlink()`, is not, because "the analysis
currently does not do any checking inside constructors or destructors". Which thread drops the
last reference is a run-time fact. So 0340 stays the job of TSan and the model.

## Clang Lifetime Safety (Clang 22.1.8, experimental), whole tree

Clang 22.1 spells it `-Xclang -fexperimental-lifetime-safety -Wexperimental-lifetime-safety`
(`-permissive`, `-strict`, `-suggestions`). The `-Wlifetime-safety-*` names in the linked page are
trunk (23) and are unknown to 22.1. The clang-tidy job already pins 22.1.8, so the clang-tidy
frontend can run it with `--extra-arg`, and no new compiler is needed.

Run over all 40 `src/*.cpp` translation units in strict mode, it gives **zero** dangling
warnings, so it costs no noise. It also gives 8 `[[clang::lifetimebound]]` suggestions, all of
them plausible:
`error_info.h:268` (`operator<<`), `ordered_map.h:357` (`FindLinear`), `helpers.h:87` and `:300`,
`render_context.h:964` (`SetTemplateFrame`), `value_visitors.h:126` (`UnwrapRecursive`) and two in
vendored `robin_hood.h`. Zero warnings also means it found nothing today. Its value is in catching
the next local dangling view and in seeding annotations, not in finding old bugs. Invalidation
checks (iterator or view after `push_back`) and cross-function inference are trunk-only and
"highly experimental".

## Other Clang tools

| Tool | Status here | Verdict |
|---|---|---|
| Static analyzer (`clang-analyzer-*`) | already in `.clang-tidy`, run on changed lines (PRs) and the whole tree (master) | keep. A full run over the 40 `src/*.cpp` with clang-tidy 22.1.8 reports nothing beyond the existing NOLINTs. CTU (cross-TU) analysis would add inter-procedural depth; see 0345 |
| `-Wunsafe-buffer-usage` (C++ Safe Buffers) | 412 distinct sites in `src/` (robin_hood 104, node_arena 40, template_parser 28, helpers 24, ...) | not now: high churn, and the bounds bugs it targets are already covered by ASan + fuzzing |
| Debug or hardened standard library (`_GLIBCXX_DEBUG`, libc++ `_LIBCPP_HARDENING_MODE=debug`) in a **test job** | not used. 0265 rejected aborting assertions for production builds | **add as a CI test job (0344)**: it catches iterator and reference invalidation in `std` containers (use after `push_back` or `erase`), the dynamic side of the lifetime class, which ASan misses while the memory is still inside the container's capacity. Aborting in a test job is a finding, not a crash in production |
| ASan container-overflow annotations | `detect_container_overflow` needs an annotated standard library (libc++, or libstdc++ with `_GLIBCXX_SANITIZE_VECTOR`) | fold into 0344 |
| MemorySanitizer | supported by CMake (`memory`), not in CI | low value for cost: it needs an MSan-built libc++ and all deps rebuilt. Uninitialised reads are rare in this code, and UBSan + `-Wuninitialized` cover the cheap part |
| `bugprone-dangling-handle` (clang-tidy) | on, with the default handle classes (`std::string_view` ...) | **configure `HandleClasses`** with our view types (0343) |
| `[[gsl::Pointer]]` / `[[gsl::Owner]]` on our own types | none | part of 0343: it lets `-Wdangling-gsl` and Lifetime Safety treat our views like `std::string_view` |
| Type sanitizer (`-fsanitize=type`, Clang 20), RealtimeSanitizer, CFI, SafeStack | none | not for debugging this project: aliasing UB is not a reported class here, and CFI and SafeStack are hardening (0265's scope) |
| A latest-LLVM analysis job (nix or the apt.llvm.org snapshot) | none | **0345**: trunk Lifetime Safety (invalidation, inference, `-Wlifetime-safety-*`), CTU static analyzer, newer thread-safety features. Report-only, so a trunk regression never blocks a merge. Older compilers gain because the annotations it validates are portable |

## Recommendation

1. **Do (0343):** add portable attribute macros, and annotate the host-API lifetime edges
   (`user_callable.h` promoters and `TypedParam`, reflection and lent-item views, the 8 suggested
   sites), plus `GUARDED_BY`/`REQUIRES` on the mutex-protected state (`TemplateEnvImpl`,
   `TemplateImpl::m_metadataMutex`, the module call mutex). Enable `-Wthread-safety` and the
   Lifetime Safety report in the existing clang-tidy job. This is small, it is checked on every
   push, and it would have caught 0296.
2. **Do (0344):** add a debug-containers test job.
3. **Do, report-only (0345):** a pinned latest-LLVM job for the trunk analyses and CTU.
4. **Do not expect** any of these to replace the model or TSan for the cell protocol. Thread
   confinement and last-reference ownership are dynamic, and memory order is outside every static
   Clang check. A cheap static guard does exist for 0339's class: a clang-query or clang-tidy rule
   that flags `use_count()` used in a condition outside an allow-list (part of 0343).
