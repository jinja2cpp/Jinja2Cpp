---
status: in-progress
priority: medium
area: robustness
depends: []
touches: [include/jinja2cpp/config.h, include/jinja2cpp/user_callable.h, src/thread_annotations.h, src/template_env_impl.h, src/template_env.cpp, CMakeLists.txt, scripts/use_count*, scripts/preflight.sh, .github/workflows/clang-tidy.yml]
shares: [src/template_impl.h, src/template_module.cpp, src/internal_value.h, include/jinja2cpp/utils/ordered_map.h, src/render_context.h, src/value_visitors.h, include/jinja2cpp/error_info.h]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/694
---
# Check lifetimes and lock discipline at compile time with Clang annotations

**Problem.** Lifetime bugs are this project's worst defect class (0112, 0115, 0125, 0134, 0296),
and we find them only at run time: ASan on a test that happens to reach the path. 0296 shipped
because no test passed a string of the other width to a typed `std::string_view` parameter. The
lock discipline of `TemplateEnvImpl` ("Guarded by `guard`") lives in comments that no tool checks.
`docs/design/clang-static-analysis-evaluation.md` shows that two annotations on the pre-fix
`user_callable.h` make Clang 20 flag the 0296 line at compile time, and that `GUARDED_BY` on
`TemplateEnvImpl` holds today with one false positive.

**Proposal.**
- Portable macros in `include/jinja2cpp/config.h` (C++17, `__has_cpp_attribute`):
  `JINJA2CPP_LIFETIMEBOUND` (`clang::lifetimebound`, `msvc::lifetimebound`, else empty) and
  `JINJA2CPP_LIFETIME_CAPTURE_BY(x)` (Clang >= 20, else empty). GCC must see nothing, because
  `-Werror` turns its unknown-attribute warning into an error.
- `user_callable.h`: `lifetimebound` on the three `operator other_string_view()` and the
  `operator string_view()` / `operator const string&()` of the promoters; a by-value
  `TypedParam::SetValue` for view types with `lifetime_capture_by(this)`; an `else` after the
  0296 early return (a false positive on dead code otherwise). Then the 8 sites Lifetime Safety
  suggests (listed in the design note), and the reflection and lent-item accessors that return views
  of their argument.
- `[[gsl::Pointer]]` on our own view types, so `-Wdangling-gsl` and Lifetime Safety treat them like
  `std::string_view`; `bugprone-dangling-handle.HandleClasses` lists them.
- `src/thread_annotations.h`: annotated `Mutex` / `SharedMutex` / scoped locks (prototype:
  project folder `clang-analysis/tsa-template-env-prototype.patch`); `GUARDED_BY` / `REQUIRES` on
  `TemplateEnvImpl::globalValues`, `m_loadSettings`, `BigIntBackendLocked`,
  `TemplateImpl::m_metadataMutex` state and the module call mutex (`std::recursive_mutex`: use
  `reentrant_capability` where the Clang version has it, otherwise leave it unannotated); an
  annotated two-lock helper for `TemplateEnvImpl::IsEqual`.
- A clang-query rule (or a small clang-tidy check) that flags `shared_ptr::use_count()` in a
  condition outside an allow-list: the class of 0339, which no Clang analysis sees.
- The clang-tidy job (already 22.1.8) adds `--extra-arg=-Wthread-safety` and the Lifetime Safety
  report (`-Xclang -fexperimental-lifetime-safety -Wexperimental-lifetime-safety-strict`), first
  as non-blocking output, then into `WarningsAsErrors` for thread safety once the tree is clean.
  The Clang 20 build jobs get `-Wdangling-capture` by default.

**Done when** a revert of the 0296 fix (`TypedParamUnwrapper` taking `Promote(v)` for other-width
text) fails the Clang 20 build with `-Wdangling-capture`; removing a lock from a `TemplateEnv`
getter fails the clang-tidy job; GCC, MSVC and AppleClang builds are unchanged.

**Next.** The annotations can lie. An ASan or TSan report on an annotated path means the
annotation is wrong and must be fixed, not the test silenced. Trunk Lifetime Safety (0345) will
check more of them.

**Resolution (PR #694).**
- `config.h`: `JINJA2CPP_LIFETIMEBOUND` (Clang only for now; MSVC's `[[msvc::lifetimebound]]` is left
  out until a Windows job checks it) and `JINJA2CPP_LIFETIME_CAPTURE_BY(x)` (Clang 20+).
- `user_callable.h`: `lifetimebound` on the three `operator other_string_view()` and on `Promote`'s
  argument; `TypedParam::SetValue` takes its value by value with `lifetime_capture_by(this)` (a
  `decayed_t&&` parameter made Clang warn for every owning type); `else` branches after the 0296
  early return. The promoters' `operator string_view()` views the argument, not the promoter, so it
  carries no annotation.
- `lifetimebound` on `FunctionRef`'s and `ReferenceWrapper`'s constructors, `UnwrapRecursive`,
  `OrderedMap::FindLinear`, `ErrorInfo`'s `operator<<` and `TemplateParser`'s name (which it keeps by
  reference: `test/name_resolver_test.cpp` passed a temporary, found by Clang 23 in 0345 and fixed
  there; the same test fix is in this PR so that the annotation does not fail the Clang builds). Lifetime Safety also suggested
  `RenderContext::SetTemplateFrame`, which returns the previous frame, not its argument: not taken.
  The iterator helpers in `helpers.h` it suggested are left out (the result is an iterator into the
  caller's range, which the annotation would not add anything to).
- `src/thread_annotations.h`: annotated `Mutex`, `SharedMutex`, `WriterLock`, `ReaderLock` and
  `ReaderLockPair` (address order, for `TemplateEnvImpl::IsEqual`). `GUARDED_BY(guard)` on
  `globalValues`, `filters`, `tests`, `translations` and both template caches;
  `m_loadSettings` / `BigIntBackendLocked` under `m_loadSettingsGuard`; `TemplateImpl::m_metadata` /
  `ParseMetadata` under `m_metadataMutex`. `InstallGettextCallables` loops instead of calling a lambda
  (the analysis checks a lambda as a function holding no lock); `GetCache` returns the cache by
  reference with `JINJA2CPP_NO_THREAD_SAFETY_ANALYSIS` (every use is under `guard`, which the
  analysis cannot follow through the reference). `-Wthread-safety` is in every Clang build's strict
  warnings (`CMakeLists.txt`), so a violation fails the build like any other warning.
- Found by the annotations: `TemplateImpl::operator==` (used when render callbacks compare their
  hosts) read the lazily filled `m_metadata` without its lock, so a `GetMetadata` on another thread
  raced it, and the comparison depended on whether `GetMetadata` had run. It now compares only
  `m_metadataInfo`, the text the cache is parsed from. `Load` clears the cache under the lock.
- Left unannotated: the module call `std::recursive_mutex` (`template_module.cpp`). Its state is
  set up before the module is published and torn down in the destructor, neither of which the
  analysis can express, and Clang 18 and 20 have no `reentrant_capability`.
- Not done: `[[gsl::Pointer]]` and `bugprone-dangling-handle.HandleClasses`. The view-like types
  (`FunctionRef`, `ReferenceWrapper`) got `lifetimebound` constructors, which give the same
  warning, and `StringView` / `BigIntView` are aggregates filled from values that own the text.
  The Lifetime Safety report moved to 0345, which runs the trunk analyses anyway.
- `scripts/use_count_check.py` (+ `use_count.query`): lists `shared_ptr` / `weak_ptr`
  `use_count()` calls in `src/` and `include/`; fails a pull request that adds one (clang-tidy
  job, preflight). On master it lists six: the three of 0339, plus `TraverseBoxed`
  (`internal_value.cpp`, two) and `LoadsInPlace` (`template.cpp`), sent to 0339's owner.
- Checked: Clang 18 and 20 `-fsyntax-only` over all 103 translation units with `-Wthread-safety
  -Wthread-safety-beta -Wdangling -Wdangling-capture -Wdangling-gsl`: no warning. Reverting the 0296
  branch (`if constexpr (false)`) gives `-Wdangling-capture` at the `SetValue(Promote(v))` line;
  removing the lock from `FindFilter` gives `-Wthread-safety-reference`.
