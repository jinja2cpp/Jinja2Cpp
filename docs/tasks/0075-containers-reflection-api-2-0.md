---
status: open
priority: medium
area: release
depends: [0071, 0069]
touches: [include/jinja2cpp/generic_list.h, include/jinja2cpp/generic_list_impl.h, include/jinja2cpp/generic_list_iterator.h, include/jinja2cpp/reflected_value.h, include/jinja2cpp/binding/, include/jinja2cpp/error_info.h, include/jinja2cpp/utils/i_comparable.h, include/jinja2cpp/filesystem_handler.h, src/generic_list.cpp]
shares: [test/]
---
# 2.0 API: containers, reflection, errors

**Problem.** Asymmetries and leaks in the extension interfaces (`docs/api-2.0.md`, 3.4-3.7):
the JSON bindings specialise `detail::Reflector`, `GenericMap` is not exported and cannot be
iterated, every accessor and filesystem handler writes `IsEqual` boilerplate, `ErrorCode`
values are implicit.

**Proposal.** Public `jinja2::Reflector<T>` (alias kept in `detail`), reflectors for every
integral type; `GenericMap` exported with key iteration; `GenericList::iterator`;
`virtual IComparable` everywhere with a default identity `IsEqual`; `MakeGenericList` in
`generic_list.h`; `BasicErrorInfo<CharT>` (alias `ErrorInfoTpl` deprecated); explicit
`ErrorCode` values; `ArgInfo::VarArgs`/`VarKwArgs`/`Context` constants and
`UserCallable::Function`.

**Done when.** The items are in, existing user-style accessors in the tests compile
unchanged, and the bindings use the public `Reflector`.
