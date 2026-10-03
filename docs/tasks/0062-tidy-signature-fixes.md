---
status: done
priority: medium
area: perf
depends: [0057, 0070]
touches: [src/, include/jinja2cpp/]
---
# clang-tidy: fixes that change signatures, copies or linkage

**Problem.** Some 0054 hits need review because the fix changes a signature or ownership:
`performance-unnecessary-value-param` (65; `FilterParams` is copied on every filter call,
`src/filters.cpp:946` and others), `modernize-pass-by-value` (19),
`performance-move-const-arg` (12), `unnecessary-copy-initialization`,
`noexcept-move-constructor`, `cppcoreguidelines-missing-std-forward` (16),
`rvalue-reference-param-not-moved`, `prefer-member-initializer`,
`readability-convert-member-functions-to-static` (26), `misc-use-anonymous-namespace`
(17), `google-explicit-constructor` (66, of which 24 in public headers),
`cppcoreguidelines-special-member-functions` (22), `readability-implicit-bool-conversion`
(16 after the allowed pointer and integer conditions).

**Proposal.** One PR per two or three checks, fixes applied with the 0054 script and
reviewed. `Value`'s converting constructors are implicit by design: they get
`// NOLINT(google-explicit-constructor)` unless 0056 decides otherwise. Measure the
`FilterParams` change with the perf benchmarks if any exist (otherwise note the
allocation count).

**Done when** these checks report nothing and sit in `WarningsAsErrors`.

**Resolution.** Four PRs, each moving its checks into `WarningsAsErrors`: 0062a
[#341](https://github.com/jinja2cpp/Jinja2Cpp/pull/341) (copies), 0062b
[#342](https://github.com/jinja2cpp/Jinja2Cpp/pull/342) (moves, forwarding and special
members), 0062c [#343](https://github.com/jinja2cpp/Jinja2Cpp/pull/343) (explicit
constructors and implicit bool), 0062d [#344](https://github.com/jinja2cpp/Jinja2Cpp/pull/344)
(static members and anonymous namespaces). Kept by design, with a `NOLINT` naming the reason:

- implicit converting constructors of `Value`, `InternalValue`, the reference wrappers,
  `ArgInfo`/`ArgInfoT`, `ArgumentInfo` and the argument promoters;
- `end()` of `GenericList`, `GenericMap` and `ListAdapter`, which stays a const member so
  the containers keep the usual container API;
- the member functions in `test/user_callable_test.cpp`, which test the member overload of
  `MakeCallable`;
- forwarding references the check misreads: `Apply`/`Apply2` build one visitor per
  alternative from the same arguments, and the user-callable helpers keep an argument's
  address or call through a reference;
- `RenderContext`'s move constructor, which copies the scopes and so cannot be
  `noexcept`, and the by-value key of the base `SetValue` that overrides store.

`FilterParams` (`CallParamsInfo`: a map of named arguments and a vector of positional
ones) is now taken by `const&`. Filters are built at parse time, plus once per call by
`map`/`select`-style filters that apply another filter, so this saves one map and one
vector copy (their node allocations plus a `shared_ptr` count bump per argument) per
filter built. The perf suite is `DISABLED_` and was not run.
