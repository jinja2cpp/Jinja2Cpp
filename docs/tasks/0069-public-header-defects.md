---
status: done
priority: medium
area: robustness
depends: []
touches: [include/jinja2cpp/config.h, include/jinja2cpp/value_ptr.h, CMakeLists.txt]
---
# Defects in the public headers

**Problem.** The 0056 API survey (`docs/api-2.0.md`, section 3) found defects that are not
naming questions and can be fixed in 1.x without changing the API. Each one marked
*checked* was reproduced with a small program against master `b0991e3`.

- `generic_list_impl.h` defines `lists_impl::MakeGeneratedList` and
  `MakeGenericList(ListGenerator)` without `inline`: two translation units that include it
  fail to link with "multiple definition" (checked). Only `test/forloop_test.cpp` includes
  it, which is why nothing noticed.
- `MakeGenericList(b, e)` with named (lvalue) iterators does not compile:
  `iterator_traits<It&>` has no `iterator_category` (checked). Only temporaries such as
  `v.begin()` work.
- `GenericList::cbegin()`/`cend()` are declared `auto` and defined in `src/generic_list.cpp`,
  so user code cannot call them ("use before deduction of auto", checked).
- `GenericList::begin()`/`end()` return `detail::GenericListIterator`, which
  `generic_list.h` only forward-declares: iterating a `GenericList` fails with "invalid
  use of incomplete type" unless the user also includes `generic_list_iterator.h`
  (found by 0064, `test/user_callable_test.cpp` keeps that include with an IWYU pragma).
- `GenericMap::GetAccessor()` calls the accessor without checking it: on a
  default-constructed `GenericMap` it throws `std::bad_function_call` (checked);
  `GenericList::GetAccessor()` checks.
- `TemplateEnv::ApplyGlobals(fn)` passes the globals as a mutable `ValuesMap&` while
  holding only the shared (reader) lock; a mutating callback compiles (checked) and races
  with concurrent readers. Pass `const ValuesMap&`. (Done in 0074.)
- `ReflectedMapImpl::GetAccessors()` returns `auto`, a copy of the accessor
  `unordered_map`; `HasValue`, `GetValueByName`, `GetKeys` and `GetSize` each call it, so
  every field access of a reflected struct copies the whole map. Return a reference.
- `ReflectedMapImplBase::GetValueByName` throws `std::runtime_error` for an unknown name,
  while `IMapItemAccessor` documents an empty `Value`.
- `JINJA2CPP_VERSION` in `config.h` is `10100` while the project is 1.3.2; generate it
  from `project(VERSION)`.
- `config.h` disables MSVC warning 4251 for the rest of every user translation unit; wrap
  it in `#pragma warning(push/pop)` around the library's own declarations.
- `value_ptr.h` includes `polymorphic_cxx14.h`, then `polymorphic.h` when
  `__cplusplus != 201402L`; both use the guard `XYZ_POLYMORPHIC_H_`, so the second include
  never does anything. It also puts `using namespace xyz;` into `jinja2::types`.
- `JINJA2_INT_REFLECTOR` stays defined after `reflected_value.h`.

**Proposal.** One PR, one commit per item, each with a test where behaviour changes
(a second TU including `generic_list_impl.h` in the test target covers the first item).

**Done when.** The items above are fixed and the checked ones have tests.

**Progress.** `ApplyGlobals` takes `const ValuesMap&` since PR #336 (task 0074). PR #334
(task 0075) fixed the items in the container and reflection headers: `inline` definitions
(now in `make_generic_list.h`, tested by two translation units), named iterators,
`cbegin`/`cend`, `GenericMap::GetAccessor`, the `GetAccessors` copy, `GetValueByName` for
unknown fields and `JINJA2_INT_REFLECTOR` (removed). The last three items are fixed
in one PR, one commit each: `JINJA2CPP_VERSION` (plus `_MAJOR`/`_MINOR`/`_PATCH` and
`_STRING`) comes from `jinja2cpp/version.h`, which CMake generates from `project(VERSION)`
and installs with the public headers; C4251 is silenced only between
`JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN`/`_END` around the library's declarations;
`value_ptr.h` includes only `polymorphic_cxx14.h` (the never-compiled C++20 `polymorphic.h`
is deleted) and names `xyz::in_place_type_t`/`xyz::polymorphic` instead of the
using-directive. 0076 still owns the inline namespace and the vendored types' future.
