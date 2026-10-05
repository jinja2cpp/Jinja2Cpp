---
status: done
priority: medium
area: robustness
depends: []
touches: [src/filters.cpp, src/internal_value.cpp]
---
# `sum(attribute=...)` never returns on a generated list

**Problem.** The verifier of the 0061 `SequenceAccessor::Filter` split found that
`{{ g|sum(attribute=0) }}` hangs when `g` is a single-pass list made with
`detail::MakeGeneratedList` (a generator lambda returning `std::optional<Value>`). It also
hangs with `attribute='v'`, `attribute='missing'`, `sum(10)` and `sum('')`. Plain
`g|sum` and the other sequence filters terminate. Master and the split behave the
same (both timed out after 10 s on a single template), so the split did not cause it.

`Sum` builds `list.ToSubscriptedList(attrName, true)`, which for a list of unknown size
wraps the generator in `CreateGenericSubscribedList` by reference; the hang is
presumably in that adapter's iteration over a single-pass source (end never reached, or
the source is restarted on every copy of the iterator).

**Done when** the case renders the same as Python's `sum(attribute=...)` over a generator,
and a unit test in `test/containers_api_test.cpp` covers it, with a timeout-free
repro (a generator over a fixed vector of values).

**Resolution.** The generic subscribed list (`CreateGenericSubscribedList` in
`src/internal_value.cpp`, used for lists of unknown size by `sum(attribute=)` and
`join(attribute=)`) called `MoveNext` only before the first item, so it returned the first
item forever. It now advances before every item. Test:
`ContainersApiTest.SumAttributeOnGeneratedList`.
