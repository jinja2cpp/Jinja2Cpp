---
status: open
priority: low
area: parity
depends: [0387, 0368]
touches: [src/value_visitors.h#BinaryMathOperation, src/value_visitors.h#FunctionRepr, src/global_functions.cpp#CallCycler, src/global_functions.cpp#CallJoiner, src/string_converter_filter.cpp, src/filters.cpp#Unique, src/serialize_filters.cpp#ToJson]
shares: [test/parity/divergences/filters.txt, test/parity/divergences/operators.txt, test/parity/divergences/globals.txt]
---
# Callables, generators and ranges: leftovers of 0387 and 0368

**Problem.** Found by the verifier of parity batch 4 group A (0387, 0368, 0371). After that PR a
generator prints, compares and serializes as Python's, and callables have Python's type names,
but these smaller differences remain (Jinja2 3.1.6):

1. `{{ [1, 2, 3]|select|truncate(5) }}`: Python raises `object of type 'generator' has no len()`;
   Jinja2C++ passes the generator through. Pin `filters.lazy_result_truncate`.
2. `{% set g = [1, 2, 3]|select %}{{ [g, g]|unique|list|length }}`: Python 1 (a generator hashes
   by identity); Jinja2C++ raises `unhashable type: 'list'`. Pin `filters.lazy_result_unique_hash`.
3. A value's bound methods compare unequal: `{{ 'a'.upper == 'a'.upper }}` is True in Python
   (same receiver, same function). Pin `operators.bound_method_equals`. Closed by #769 (0212 P1)
   once #774 landed: the case now matches and its allowance is removed.
4. A range acts as a list: `range(3) == [0, 1, 2]` is True, `range(3) + [1]`, `range(3) * 2`,
   `range(3) < range(4)` work, and `range(2)|tojson` gives `[0, 1]`; Python's range equals only a
   range and has none of these. Pins `operators.range_equals_list`, `operators.range_concat`,
   `filters.range_tojson`.
5. A cycler's or joiner's method (`cycler(1).next`, `.reset`, `joiner().__call__`) prints nothing;
   Python prints `<bound method Cycler.next of <jinja2.utils.Cycler object at 0x...>>`. `loop.cycle`
   already prints so (0305). Pin `globals.cycler_method_prints`.
6. Not pinned (no stable output): `{{ self.b }}` prints nothing and its undefined attributes name
   `jinja2.runtime.Macro`; Python has `<jinja2.runtime.BlockReference object at 0x...>`.
   `{{ lipsum.__class__ }}` is undefined; Python prints `<class 'function'>`. A reflected host
   struct is named `dict object` in undefined messages.

Host callables print nothing on purpose (0368: their Python repr would be the host's, with an
address that differs between runs), and a generator replays (0101, deliberate).

**Proposal.** 1-2: check `GetIteratorKind()` where truncate takes the length and where unique
hashes. 3: compare `BoundMethodCall`'s receiver identity and `invoke` in `Callable::IsSame`
(coordinate with the 0212 thread, which owns `Callable::IsSame`). 4: in `BinaryMathOperation`, treat
a list with `GetRangeInfo()` as a range: equal only to a range with the same items, no + or *,
ordering a TypeError; tojson refuses it. 5: give the cycler and joiner methods a bound receiver
(`SetBoundMethod` with an `IBoundReceiver`), as `loop.cycle` has.

**Done when** the remaining six 0401 lines are gone from `test/parity/divergences/`.
