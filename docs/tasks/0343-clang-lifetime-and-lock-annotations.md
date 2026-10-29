---
status: open
priority: medium
area: robustness
depends: []
touches: [include/jinja2cpp/config.h, include/jinja2cpp/user_callable.h, src/thread_annotations.h, src/template_env_impl.h, src/template_env.cpp, .clang-tidy, .github/workflows/clang-tidy.yml]
shares: [src/template_impl.h, src/template_module.cpp, include/jinja2cpp/utils/ordered_map.h, src/helpers.h, src/render_context.h, src/value_visitors.h, include/jinja2cpp/error_info.h]
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
