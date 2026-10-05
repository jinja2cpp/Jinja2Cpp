---
status: in-progress
priority: medium
area: perf
touches: [src/internal_value.h#ValuesMapAdapter, src/internal_value.cpp#ListAdapter, include/jinja2cpp/value.h#IMapItemAccessor, src/filters.cpp#ToValueList]
---
# Reading user data builds keys and copies lists

**Problem.** Every template reads the caller's data through adapters
(`ValuesMapAdapter`, `ValuesListAdapter`, `ListAdapter`), and the adapters' interfaces
make the reader pay:
- `IMapItemAccessor::GetValueByName(const std::string&)` and `HasValue(const
  std::string&)` take an owning string, so `Subscript(value, name)` builds one per
  access; `Render/many_tags`: `Subscript(.., std::string const&)` 12%,
  `ValuesMapAdapter::GetItem` 6%;
- a filter that wants a list calls `ListAdapter::ToValueList()`, copying every item
  (`Render/strings` 17%, together with 0114's `%` arguments);
- enumerating a user list converts each `Value` to an `InternalValue`
  (`Render/mitsuhiko_table`: `GetCurrent` 5%, the enumerator 4%).

The contradiction: the adapters keep user data unconverted (cheap to pass in), but
their API forces a conversion at every read. The public interface is about to change
anyway for 2.0 (0072): additive `string_view` overloads can land first, and 0072 folds them into the final shape.

**Proposal.** Needs an architect plan first (public API):
1. `string_view` (heterogeneous) lookup through the map accessors, with the
   `std::string` overloads kept as `[[deprecated]]` forwards as the 2.0 plan does.
2. Filters iterate a list through its accessor instead of `ToValueList()` where they
   only read it once (`join`, `map`, `select`, `sum`, `length`...).
3. Borrow scalar items (string views, numbers) from user lists instead of converting
   (0100 rejected the cheap version at -2.6%; revisit with 1-2 in place).

**Done when.** `Render/many_tags` -10% and `Render/strings` -10% instructions, no API
break without a deprecated forward.

**Progress.** Plan (approved by Ruslan 2026-10-05): the profile showed `many_tags` builds no
key (the attribute name is already a `std::string` in the AST); its cost was the
`SubscriptExpression` call path. So the work splits into:
- PR #390 (merged): callable-returned lists and maps lent their strings as views and nested
  containers by reference with nothing keeping the owner alive (heap-use-after-free); also
  fixed 0112.
- Lists and attribute path (this PR): `IListAccessor::ForEach`, `ToValueList` with
  `reserve`, filter loops over `ForEach` (also makes `unique` read single-pass lists once),
  a one-index fast path in `SubscriptExpression`, `ResolveCallOperator` by reference.
  `Render/many_tags` -11.3%, `Render/strings` -7.7% (0114's `%` change took its share
  first), `for_filter_if` -14.7%, `for_loop_vars` -11.5%, nothing slower.
- Public `IMapItemAccessor::Find`/`Contains(std::string_view)`, with the 1.x
  `HasValue`/`GetValueByName` kept (not deprecated) and bridged both ways: next PR.

