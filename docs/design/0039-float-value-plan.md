# 0039 plan: C++ `float` in `jinja2::Value`

Architect plan for docs/tasks/0039 (master `d79e11c`, 2026-10-24; rebased on `3986280` in
section 9). Decision it serves: D21 in docs/release-2.0.md (Ruslan, 2026-10-24: add a `float`
alternative to `Value`, accept the ABI change, land before the G1 freeze).

**Approved by Ruslan on 2026-10-27: D1-D8 as recommended.** Phases: 0332 (MakeCallable
per-argument dispatch), 0333 (locale-free float parsing), then 0039 itself.

## 1. The problem in one picture

A C++ `float` is a ruler with millimetre marks; a `double` is the same ruler with micrometre
marks. When the embedder writes `12.123f`, the float ruler can only stop at the nearest mark,
`12.12300014495849609375`. Today `Value` widens that to the fine ruler *exactly*, so the
template, which prints the shortest text that identifies a **double**, prints
`12.123000144958496`: it faithfully reports a millimetre mark measured in micrometres.

Storing the `float` in `Value` is only half the fix. The other half is what the *template*
does with it, because inside the template the number meets `*`, `+`, `==`, `|string`,
`|tojson` and `'%s'|format`. That is the central decision (D1) and it decides almost
everything else, including how big the change is.

## 2. What exists today (checked on master)

- `Value::ValueData` (include/jinja2cpp/value.h:325) has 16 alternatives, ending
  `..., RecWrapper<UserCallable>, Markup, WMarkup, BigInt`. `Value(F)` for any floating type
  stores `static_cast<double>(val)`; `test/basic_tests.cpp:989` pins `Value(1.5f)` as double.
- `Reflector<float>` (include/jinja2cpp/reflected_value.h:669) makes `Value(double)`.
- Inside the engine `InternalValue`'s `ValueStorage` (src/internal_value.h:375) has one
  float kind, `double`. About 100 sites test it by type (`GetIf<double>`: 44 in
  value_visitors.h, 38 in filters.cpp, more in python_format, serialize_filters,
  value_methods, template_dict, testers...).
- Printing: `PythonFloatText` (src/value_visitors.h:255) = fmt shortest digits plus Python's
  `repr` rules. `Value::ToString` goes through `InputValueConvertor` and the same printer.
- `Value::IsEqual` is `m_data == rhs.m_data`: structural, so `Value(1) != Value(1.0)` already.
- Tests encoding the widened digits: `test/expressions_test.cpp:90,120,125,443`
  (`{"doubleValue", 12.123F}` -> `12.123000144958496`), `test/basic_tests.cpp:989`.

### MakeCallable blow-up, measured

The untyped path (`MakeCallable(lambda, ArgInfo{...}...)`, include/jinja2cpp/user_callable.h:292)
does one `std::visit` over all N argument variants, instantiating `UCInvoker` and its SFINAE
probe for every combination: 16^N today, 17^N with `float`. One TU with a single lambda of
N `const std::string&` parameters (g++ -O2, this container):

| N | compile | .text |
|---|---|---|
| 0 | 1.7 s | 0.1 KB |
| 1 | 2.3 s | 25 KB |
| 2 | 4.6 s | 170 KB |
| 3 | 40.9 s | 2.4 MB |

N = 4 extrapolates to about 10 minutes and 35 MB for one lambda. The typed path
(`ArgInfoT<T>`, function pointers, member pointers) already visits per argument (N x 16).

## 3. Python behaviour (the oracle has no float32; numpy is the nearest analogue)

Jinja2 3.1.6, three inputs for "the embedder's `12.123f`": numpy's `float32`, the exact
widening (S1), and the double nearest the float's shortest decimal (S2):

| template | np.float32 | S1 exact widening | S2 decimal widening |
|---|---|---|---|
| `{{ x }}` | 12.123 | 12.123000144958496 | 12.123 |
| `{{ x * 2 }}` | 24.246 | 24.246000289916992 | 24.246 |
| `{{ x == 12.123 }}` | True | False | True |
| `{{ x - 12.123 }}` | 0.0 | 1.4495849676166017e-07 | 0.0 |
| `{{ x\|string }}`, `'%s'\|format(x)` | 12.123 | 12.123000144958496 | 12.123 |
| `{{ [x] }}` | [np.float32(12.123)] | [12.123000144958496] | [12.123] |
| `{{ x\|tojson }}` | TypeError | 12.123000144958496 | 12.123 |
| `{{ [x, 1.5]\|sum }}` | 13.623 | 13.623000144958496 | 13.623 |
| `{{ '%.10f'\|format(x) }}` | 12.1230001450 | 12.1230001450 | 12.1230000000 |
| `{{ x is float }}` | False | True | True |

(Probe run in the plan thread with Jinja2 3.1.6 on Python 3.13; float repr is the same on the pinned 3.12.)

S2 matches what a Python user (and numpy) sees in every row but the `%.10f` one, which shows
digits below the float's precision. S1 fixes only `{{ x }}` itself (and only with an extra
float print kind); every derived value is noisy again.

## 4. Design (approved options)

### 4.1 In the template a `float` *is* the Python float its digits name (D1 = S2)

`InputValueConvertor` converts a `float` to the `double` nearest its shortest round-trip
decimal (12.123f -> 12.123), once, at the boundary. From there it is an ordinary `double`:

- **No new `InternalValue` alternative**, none of the ~100 `GetIf<double>` sites change, no
  visitor grows, the per-node render path is untouched (perf risk on the hot path: none).
- `{{ x }}` prints `12.123` with the existing double printer: a decimal of at most 9
  significant digits round-trips through `double`, so its shortest double text is itself.
- `(float)d == f` for the converted `d`, so a typed `float` callable parameter gets the
  exact original float back (`ArgPromoter<double>` casts).
- Verified exhaustively over all 2^32 bit patterns (see 4.6). Exactly two floats,
  `±7.038531e-26`, suffer double rounding: the double nearest their shortest decimal rounds
  to the *neighbouring* float. Rule: if `(float)d != f`, take one more digit
  (`7.0385307e-26`), which round-trips and prints as itself. A unit row pins both.

Cost: one shortest-digits pass (fmt/Dragonbox) and one exact decimal->double per float value
entering a template. Reflected `float` members convert on each access, as all reflected
values do. Nothing changes for `double`.

### 4.2 Public `Value`

- `float` is **appended** as alternative 16, after `BigInt` (D2). Existing indices stay put;
  `sizeof(Value)` is unchanged (static_assert already pins it; add one for the index).
- `Value(float)` stores `float`; `Value(double)` and `Value(long double)` store `double` (D8).
- Accessors, exact as for `BigInt` vs `int64_t` (D3): `IsFloat()`, `AsFloat()`,
  `AsFloat(float&) noexcept`, `As<float>()`, `GetIf<float>()`, `jinja2::Visit` sees `float`.
  `IsDouble()` stays false for a float.
- Equality structural (D7): `Value(1.5f) != Value(1.5)`, as `Value(1) != Value(1.0)` today.
  Templates compare numerically, as always.
- `Value::ToString(12.123f)` = `"12.123"` (it already goes through the convertor).
- Other public visitors get an explicit `float` case: `error_info.cpp` `ValueRenderer` (three
  visits), `string_helpers.h` getters (generic fallback is already right: empty string),
  `ParamUnwrapper`/`TypedParamUnwrapper` (fundamental promoter covers it).
- `Reflector<float>` makes `Value(float)`; `std::vector<float>` etc. follow.
- JSON bindings: unchanged (JSON numbers are doubles; simdjson 0302 likewise).

### 4.3 What comes back out of a template (D4)

A value computed or passed through a template returns to C++ as `double` (`IntValue2Value`
unchanged): `{{ cb(x) }}` with an untyped `cb(const Value&)` sees `IsDouble()`, value 12.123.
A typed `float` parameter recovers the exact float (4.1). Keeping a "was float" mark through
the engine would need a flag in `ValueStorage` or a 19th alternative; rejected for now, it can
be added later without an ABI change if a user needs it.

### 4.4 MakeCallable per-argument dispatch (D5) — lands first, own task

- A callable with a deducible signature (non-generic lambda or functor with one
  `operator()`, function pointer, member pointer) gets its parameter types from a callable
  traits helper and each argument is visited **alone** (N x 17), keeping today's untyped
  contract: unwrapped alternative + `Promote`, `None` promotes to `T()`, an argument that
  cannot convert makes the call return `None`. A `const Value&` parameter receives the
  `Value` itself instead of a copy rebuilt from the alternative.
- A generic callable (`auto` parameters, overload sets) cannot be deduced: keep the joint
  visit for up to 2 untyped arguments (289 instantiations, measured 4.6 s), and fail at
  compile time from 3 with a `static_assert` that says "describe the parameters with
  ArgInfoT<T>". The only such use in the tree (`test/user_callable_test.cpp:168`) already
  uses `ArgInfoT`.
- Acceptance: the N = 3 probe above drops from 41 s / 2.4 MB to the N x 17 level (target
  < 5 s); `test/user_callable_test.cpp` passes unchanged; new rows for a 4-argument untyped
  lambda, a `const Value&` parameter, a mismatched argument returning None, the static_assert
  (as a compile-fail check only if the harness already has one; otherwise documented).

### 4.5 Exact decimal -> double without the C locale (D6)

The conversion in 4.1 must not use `strtod`: it follows `LC_NUMERIC`, so a host that calls
`setlocale(LC_ALL, "")` under a decimal-comma locale would read "12.123" as 12. That bug
already exists outside 0039: the lexer (`src/lexer.cpp:78`), `float` filter and others call
`strtod` (filed as a new task, see 6). For 0039:

- Fast path (Clinger): shortest digits give significand `s <= 999999999` and exponent `e`;
  for `|e| <= 22` the double `s * 10^e` (or `s / 10^-e`) is one correctly rounded operation.
  This covers floats of magnitude roughly 1e-13 to 1e30 (the usual range) with no parser at all.
- Outside it: the shared locale-free parser from the new task. Recommended: vendor
  `fast_float` (header-only, MIT/Apache/BSL-1.0, the code libstdc++ 12 itself uses for
  `from_chars`), pinned + hashed in `thirdparty/internal_deps.cmake` and in `sbom/`.
  `std::from_chars(double)` is not an option yet: older AppleClang libc++ lacks it
  (task 0083 notes this).

### 4.6 Exhaustive check

`float -> fmt "{}" -> strtod -> (float)` and `fmt(double) == fmt(float)` over all 2^32 bit
patterns (fmt 11 shortest digits, glibc strtod, 9 minutes): 4,278,190,080 finite floats;
2 double-rounding failures (`±7.038531e-26`, fixed by the one-more-digit rule above, which
then round-trips exactly); 0 digit mismatches otherwise. Side finding: fmt prints a *float*
in exponent form from 1e7 (`1e+07`) where Python's repr of the same number is
`10000000.0`; printing the converted double through the existing `PythonFloatText`
avoids that, so 0039 needs no float printer of its own. CI gets a sampled version (every 4099th pattern plus edges:
subnormals, FLT_MAX, FLT_MIN, -0.0f, 1e16f boundary, 1e-5f boundary, inf, nan); the
exhaustive run is a one-off recorded in the PR.

## 5. Phases

| Phase | Task | Content | Files | Verifier | Gate |
|---|---|---|---|---|---|
| P0 | 0332 (new) | MakeCallable per-argument dispatch (4.4) | include/jinja2cpp/user_callable.h, test/user_callable_test.cpp | yes | compile-time probe; perf gate n/a (header) |
| P1 | 0333 (new) | Locale-free float parsing: fast_float, replace `strtod` sites, LC_NUMERIC test | thirdparty/, sbom/, src/lexer.cpp, src/filters.cpp, src/big_int.cpp, src/value_methods.cpp, src/json_reader.cpp | yes | `--perf` (lexer) |
| P2 | 0039 | `float` in `Value` (4.1-4.3), depends on P0 and P1 | include/jinja2cpp/value.h, reflected_value.h, string_helpers.h, src/value_visitors.h (convertor), src/error_info.cpp, test/basic_tests.cpp, test/expressions_test.cpp, docs/api-2.0.md, README changelog, abi/ | yes | `--perf`; abidiff shows only the new alternative |

P0 and P1 are independent (disjoint files) and can run as parallel threads; P2 starts when
both merged. If Ruslan prefers fewer PRs, P2 can carry P1's fast path only (Clinger + a
locale-safe fallback) and P1 follows; I do not recommend it, since the fallback would be the
same parser P1 adds.

ABI: `ValueData`'s template arguments change, so every symbol mentioning `ValueData`
(`GetData`, `Visit` instantiations in user code) changes; layout and size do not. Old binaries
fed a float `Value` would hit an unknown index, which is why it is a major-version change
and why it must be in 2.0 before G1. abidiff report goes in the P2 PR.

Perf: no change to `InternalValue`, so the render path is untouched; `bench/count.py` must
show 0 on the existing suites. One new micro row: rendering 1000 reflected `float` members
(conversion cost, reported, not gated).

## 6. Decisions (recommendation first; all approved as recommended on 2026-10-27)

- **D1 template semantics**: (a) S2 decimal widening at the boundary — **recommended**,
  Python- and numpy-like in every row, zero engine change; (b) S1 exact widening plus a
  float print kind in `InternalValue` — only `{{ x }}` gets clean, ~100 sites to touch,
  visitor growth on the hot path; (c) float arithmetic in float32 (numpy NEP 50) — not
  Python, rejected.
- **D2 position**: (a) append after `BigInt` — **recommended**, indices stable; (b) next to
  `double` — reads nicer, renumbers 8-16.
- **D3 accessors**: (a) exact `IsFloat/AsFloat`, `IsDouble` stays exact — **recommended**
  (the `IsInt`/`IsBigInt` rule); (b) `IsDouble`/`AsDouble()` also accept a float (by value
  only; `As<double>`/`GetIf<double>` cannot return a reference to a widened float, so the
  rule would be half-applied).
- **D4 out of the template**: (a) `double` — **recommended**; (b) keep a float mark through
  the engine.
- **D5 MakeCallable generic callables**: (a) joint visit up to 2 untyped args, static_assert
  from 3 — **recommended**; (b) require `ArgInfoT` for every generic callable; (c) keep the
  joint visit (17^N).
- **D6 parser**: (a) Clinger fast path + vendored fast_float, shared with the locale fix —
  **recommended**; (b) `std::from_chars` (needs a newer AppleClang floor); (c) `strtod_l` /
  `_strtod_l` per platform.
- **D7 host equality**: (a) structural, `Value(1.5f) != Value(1.5)` — **recommended**,
  matches int/double today; (b) numeric.
- **D8 long double**: (a) stays `double` — **recommended**; (b) its own alternative (no
  Python counterpart, ABI-unstable across platforms).

## 7. Overlap with work in flight

- No open PR touches `include/` (checked #643, #645, #646, #647 on 2026-10-24).
- #645 (call protocol 0220/0294/0214) and #647 (output/format 0293...) both edit
  `test/user_callable_test.cpp`; #646, #643, #647 edit `src/filters.cpp`; #645 and #643 edit
  `src/internal_value.{h,cpp}`. P0 and P2 only add test rows there: textual rebase only.
  P1 replaces `strtod` calls in `src/filters.cpp`, so it should start after those merge or
  rebase onto the train.
- 0212 (objects as keys) `touches` value.h and internal_value: if it runs at the same time
  as P2, whichever lands second rebases.
- 0230 Boost removal: fast_float is not Boost and adds one small header dependency; SBOM entry
  in P1.
- 0077 (migration script): add a note that `Value(float)` is no longer `IsDouble()`.
- Parity corpus: unaffected (its context comes from JSON, no float32).

## 8. Findings filed with this plan (findings policy)

Filed in the PR that adds this file.

- 0332 MakeCallable per-argument dispatch (P0 above).
- 0333 `strtod` follows `LC_NUMERIC`: float literals, `float` filter and friends misread
  under a decimal-comma locale (P1 above). Pin: a unit test that sets `LC_NUMERIC` to a
  decimal-comma locale when one is installed, skipped otherwise.

Numbers renumbered on 2026-10-25 (0311 went to the JSON components task); re-check master
and open PRs again before the docs PR.

## 9. Rebased on 3986280 (2026-10-25)

Checked master `3986280` against the plan's base `d79e11c` (0230 Boost-free core #641-#655,
JSON components 0311 #658, RapidJSON removal #659, call protocol #645, statements #643,
train 33 #649, 0213 #642, 0284 #634). The design and D1-D8 are unchanged.

- **Value::ValueData**: unchanged, still 16 alternatives ending in `BigInt`; `float` is
  still appended as index 16.
- **user_callable.h**: #645 changed only `test/user_callable_test.cpp`; the header is
  byte-identical to `d79e11c`, so P0 (now 0332) stands as written. Its new test rows go
  next to #645's.
- **JSON (0311 `json_adapter.h`)**: a JSON number with a fraction stays `double`
  (`JsonKind::Double` -> `Value(double)`), as do nlohmann and Boost.JSON `AsDouble`. JSON has
  no float32, so `JsonKind` needs nothing new. If a custom `JsonTraits::AsDouble` returns
  `float`, it is widened by `static_cast<double>` at json_adapter.h:548: correct (JSON text
  names a double), no change.
- **Locale**: the new internal `{% meta %}`/JSON reader (`src/json_reader.cpp:579`, 0230)
  already works around `strtod`'s locale by swapping in `localeconv()->decimal_point`, which
  confirms the hazard; P1 (now 0333) replaces that workaround with the shared parser too.
  The other `strtod` sites (lexer.cpp:78, filters.cpp x4, big_int.cpp:368) are unchanged.
- **fast_float (P1)**: a new dependency: pin + `URL_HASH` in `thirdparty/internal_deps.cmake`
  following #655's immutable-pin rules, an entry in `sbom/components.json`, and the
  conan/external dependency modes. Fallback if Ruslan prefers no new dependency: a small
  exact decimal->double (Eisel-Lemire + big-decimal fallback) in `src/`, about 400 lines.
- **Renumbering**: MakeCallable per-argument dispatch is now **0332**, locale-free float
  parsing **0333** (reserved by the backlog thread on 2026-10-27; 0311 = JSON components,
  and 0312/0313 went to #667, task 0301's module fixes).
- **Overlap**: no open project PRs remain; section 7's PR list is historical.

## 10. Since approval (master `1256084`, 2026-10-27)

- `include/jinja2cpp/user_callable.h` changed in 0296 (`MakeCallResult` copies returned views,
  `ConvertedTextStorage` for view parameters of the other width). 0332 builds on that header.
- Two open tasks edit the same dispatch: 0319 (`MakeCallable` with a `Value` parameter does
  not compile) and 0320 (lambda and typed paths differ on an omitted argument). 0332's
  per-argument dispatch gives a `Value` parameter the argument unchanged (4.4), which is
  0319's fix, and has one argument contract for both paths, which is 0320's. They should
  land in the same PR as 0332, or right after it in the same thread; running them in
  parallel threads would conflict in every hunk.
- `Value::ValueData` is still 16 alternatives ending in `BigInt`.
