---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/334
priority: medium
area: release
depends: [0071, 0069]
touches: [include/jinja2cpp/generic_list.h, include/jinja2cpp/generic_list_impl.h, include/jinja2cpp/make_generic_list.h, include/jinja2cpp/value.h#GenericMap, include/jinja2cpp/value.h#ArgInfo, include/jinja2cpp/value.h#UserCallable, include/jinja2cpp/user_callable.h, src/template_impl.h#ErrorInfoTpl, include/jinja2cpp/generic_list_iterator.h, include/jinja2cpp/reflected_value.h, include/jinja2cpp/binding/, include/jinja2cpp/error_info.h, include/jinja2cpp/utils/i_comparable.h, include/jinja2cpp/filesystem_handler.h, src/generic_list.cpp]
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

**Outcome.** Done in PR #334, with the 0069 items that live in these headers. Two
deviations from the design: `detail::Reflector` is not an alias but the base of the
primary `jinja2::Reflector<T, Tag>` template, because an alias template cannot be
specialised and specialising is what 1.x users do with it; and `GenericMap` iterates
`std::pair<std::string, Value>` (not `pair<const std::string, Value>` as `std::map`
does), so the iterator stays copy-assignable. `MakeGenericList` went to a new
`make_generic_list.h` rather than `generic_list.h`, which cannot see `Value` (`value.h`
includes it); `generic_list_impl.h` forwards to it. Found on the way and fixed: a
generated list's `IsEqual` called both generators, and `Clone()` of a forward or
random-access enumerator that had not started iterated nothing.
