# 0178 plan: integers of any size, behind a backend interface

Architect plan for docs/tasks/0178 (master `48f6983`, 2026-10-18). Inputs: 0178 incl. "Decided by
Ruslan 2026-10-18"; 0172 and PR #573 (`detail::ToStoredInt` throws `std::overflow_error`); 0015;
0230/0229 (#570, #574); docs/release-2.0.md G1, D1-D3; docs/api-2.0.md 2, 3.1, 5.4;
/mnt/project-files/release-2.0/0076-plan.md (inline `v2`, abidiff); docs/design/0140-internal-value-plan.md;
0250 (on the #573 branch) and 0252 (#575). Python Jinja2 3.1.6 / CPython 3.13 is the oracle for
every behaviour quoted below (run in this thread).

## 0. Decisions in one screen

1. **Runtime backend, compile-time default.** A small, versioned, C-like function table
   (`jinja2::BigIntBackend`, public header) that the core calls only after an int64 overflow.
   Each `TemplateEnv` has one. `-DJINJA2CPP_WITH_BIGINT=none|boost` only picks the built-in default
   that every env starts with. The Boost adapter is a public header-only implementation of that
   table (`jinja2cpp/big_int/boost_cpp_int.h`). It doubles as the sample, and users of a
   prebuilt `none` library can register it without rebuilding the library.
2. **Representation is separate from arithmetic.** A big integer always has a backend-neutral
   form: canonical decimal text. The public `jinja2::BigInt` holds it. So does the core's
   "text form", which exists in **every** build. Without a backend, big values print, compare,
   serialise, convert to float and change base exactly. Only arithmetic between integers that
   leaves int64 raises, as 0015 does today.
3. **Layout is identical in every build.** `Value` gains one alternative, `BigInt`, appended
   last. `sizeof(Value)` does not change (BigInt is one `std::string`, the size of an existing
   alternative). `InternalValue` stays 32 B (`src/internal_value.h:1225`), with one more trivially
   copyable 8 B alternative. No public macro depends on the backend.
4. **Hot path unchanged.** Promotion replaces the existing `ThrowIntegerOverflow()` calls
   (`src/value_visitors.h:1000`) with a cold out-of-line call. A computed big result that fits
   int64 is demoted at once, so "big" always means "outside int64". Mixed int64/big comparisons then
   need only the big value's sign.
5. **Only the public `BigInt` value (0269) blocks G1.** The backend interface, the
   `TemplateEnv` methods and the Boost backend only add to the ABI: a new header, new exported
   functions, and a struct versioned by its size. They are in 2.0 by D2, but a slip would ship them
   in 2.1 with no SOVERSION bump.

## 1. The problem as contradictions

**C1 (main): one binary, many arithmetics.** A prebuilt `libjinja2cpp` (Conan, vcpkg, distro)
must be one fixed artifact, with one ABI, one SBOM and no Boost in the default build (0230, D3).
At the same time the arithmetic must be the user's choice: none, Boost, GMP, CPython's own `int`
(0248), or something of their own (Ruslan: "user can choose on itself by providing its own
implementation"). A compile-time policy settles this by multiplying binaries: every choice is a
different library and a different package id. A custom backend then means building from source.
A runtime interface settles it by moving the choice to data: one binary, and the table's shape is
what gets frozen. It is a socket standard against wiring the appliance into the wall. The cost of
the socket, an indirect call, falls only on the path that is already slow (an overflowed
integer). That is why the separation costs nothing measurable.

**C2: every integer operation may promote, but integer operations must pay nothing.** It is
settled where 0015 already put the check: `AddOverflows`/`SubOverflows`/`MulOverflows`
(`src/value_visitors.h:947-997`) already branch on overflow. Only the body of that branch changes,
from throwing to promoting. Code that never overflows runs the same instructions.

**C3: `Value` must carry any host integer in every build, but the default build has no
big-integer library.** D1 (0172) asks for "render the value or fail loudly". The settlement is to
split representation (decimal text: anyone can hold, show and compare it, like cash) from
arithmetic (the backend, like a bank: needed only to compute). With that split, a `uint64_t`
above `INT64_MAX` renders exactly in the default build, and only `x * 2` raises.

## 2. Options for backend selection (C1)

| | A. Compile-time policy | B. Runtime table (recommended) | C. Boost-or-nothing switch |
|---|---|---|---|
| Shape | `-DJINJA2CPP_BIGINT=none\|boost\|custom` + `-DJINJA2CPP_BIGINT_HEADER=<path>` defining a type with the ops; core instantiates `ValueBox<Integer>` | `struct BigIntBackend` of function pointers, `TemplateEnv::SetBigIntBackend`; option picks the built-in default | `-DJINJA2CPP_WITH_BIGINT=boost` compiles cpp_int in; no user hook |
| Prebuilt package | one per choice (Conan option → package id; vcpkg feature); custom = always from source | one binary serves all; boost-built variant optional | one per choice |
| User backend into a prebuilt lib | no | yes, at run time | no (fails Ruslan's third point) |
| Value / InternalValue layout across builds | same (box is a pointer); BigInt public type must still be backend-neutral | same | same |
| Frozen public surface | `BigInt` only (smallest) | `BigInt` + `BigIntBackend` v1 (13 function pointers) + 2 env methods + 1 free function | `BigInt` only |
| Cost per promoted op | inlined ops | 1 indirect call per op (noise next to the allocation a big result needs anyway) | inlined |
| Test matrix | three library variants, custom path untested by us | the interface is tested in **every** CI row by a test-only backend; Boost adds rows | two variants |
| Header ODR risk | the header changes per build, unless kept out of public headers | none: public headers identical in every build | none |

A's only real advantage is the smaller freeze. It buys inlining on a path that has already
allocated, and pays for it with the prebuilt story and with "users can supply their own". B
freezes more, but the table is versioned by `structSize`, so it can grow in 2.x the way
`cbSize`-style C APIs grow. **Recommend B.**

## 3. Recommended design

### 3.1 Public value (0269, G1)

New `include/jinja2cpp/big_int.h`, included by `value.h`. Inside `jinja2::v2` once 0076 PR-2 has run,
and listed in `fwd.h`/`jinja2cpp.h`:

```c++
//! An integer of any size, as Python's int. Backend-neutral: holds the canonical decimal form
class BigInt
{
public:
    BigInt() = default;                          // 0
    explicit BigInt(int64_t value);
    explicit BigInt(uint64_t value);
    //! "[-]digits", no leading zeros except "0"; nullopt otherwise (failure contract: 0250)
    static std::optional<BigInt> Parse(std::string_view decimal);
    [[nodiscard]] const std::string& ToString() const noexcept;   // as Python str()
    [[nodiscard]] int Sign() const noexcept;
    bool ToInt64(int64_t& out) const noexcept;   // false when outside int64
    bool ToUInt64(uint64_t& out) const noexcept;
    friend bool operator==(const BigInt&, const BigInt&) noexcept;  // and != < <= > >=, by value
private:
    std::string m_decimal;
};
```

`Value` (`include/jinja2cpp/value.h:293-306`):
- `ValueData` appends `BigInt` after `RecWrapper<UserCallable>`. The indices of existing
  alternatives stay as they are. Code that calls `std::visit` exhaustively over `GetData()` without
  a generic fallback stops compiling. That is a release note (MIGRATION, 0077), and the reason this
  must be in 2.0.0: a new alternative after the tag changes a user-allocated type and bumps
  SOVERSION (api-2.0.md 5.4). If 0180 also appends an alternative, it goes after this one, or the
  other way round, whichever lands second.
- `IsBigInt()`, `AsBigInt()` (throws), `bool AsBigInt(const BigInt*& out) noexcept` (a pointer,
  as api-2.0.md section 2 says for large objects), and `GetIf<BigInt>()`. `IsInt()`/`AsInt()` keep
  meaning `int64_t`, so existing code does not change meaning.
- The integral constructor (`value.h:395`, on #573 `detail::ToStoredInt`, branch `value.h:271-287`)
  stores a `BigInt` instead of throwing when the value is outside int64.
  `noexcept(!detail::CanExceedInt64<I>)` stays (a 20-digit string allocates). The same applies to
  `Reflect` (`reflected_value.h`) and the three JSON bindings, which #573 routed through
  `ToStoredInt`. The throw added by 0172 therefore never reaches a release (D1: "leaves room for
  0178 to turn the throw into a big integer without a source break").
- `ToString(Value)` prints the digits. `Value == Value` compares BigInts by value.
- A public `BigInt` may hold a small number (`BigInt(5)`): normalisation happens when it enters a
  template (3.3).

### 3.2 Backend interface (0271; public, additive)

New `include/jinja2cpp/big_int_backend.h`:

```c++
enum class BigIntStatus : int32_t { Ok = 0, DivisionByZero, OutOfMemory, TooLarge, Unsupported, BadInput };

//! A big-integer implementation. Grows only at its end; the library reads structSize.
//! Every function is noexcept and reports failure by status. `out` is uninitialised storage of
//! objectSize bytes aligned to objectAlign; on Ok it holds a constructed object the library
//! later passes to destroy. Objects are immutable once made; calls on distinct or shared
//! objects may come from several threads at once. A backend must outlive every render that
//! uses it (static storage is the supported case).
struct BigIntBackend
{
    uint32_t structSize;   // sizeof(BigIntBackend) as the backend was compiled
    uint32_t version;      // 1
    const char* name;      // "boost-cpp_int"; shown in errors and by GetBuiltinBigIntBackend()
    uint32_t objectSize;   // <= 64
    uint32_t objectAlign;  // <= 16
    void (*destroy)(const BigIntBackend* self, void* obj) noexcept;
    BigIntStatus (*fromInt64)(const BigIntBackend* self, void* out, int64_t value) noexcept;
    BigIntStatus (*fromDecimal)(const BigIntBackend* self, void* out, const char* digits, size_t size) noexcept; // canonical "[-]digits"
    bool (*toInt64)(const BigIntBackend* self, const void* obj, int64_t* out) noexcept;
    BigIntStatus (*toDecimal)(const BigIntBackend* self, const void* obj, void (*sink)(void* ctx, const char* data, size_t size), void* ctx) noexcept;
    int (*sign)(const BigIntBackend* self, const void* obj) noexcept;
    int (*compare)(const BigIntBackend* self, const void* a, const void* b) noexcept;
    size_t (*bitLength)(const BigIntBackend* self, const void* obj) noexcept;    // of |x|
    BigIntStatus (*negate)(const BigIntBackend* self, void* out, const void* x) noexcept;
    BigIntStatus (*add)(const BigIntBackend* self, void* out, const void* a, const void* b) noexcept;
    BigIntStatus (*subtract)(const BigIntBackend* self, void* out, const void* a, const void* b) noexcept;
    BigIntStatus (*multiply)(const BigIntBackend* self, void* out, const void* a, const void* b) noexcept;
    //! Truncating division, as C++ and most libraries do it; the library floors it. Either output may be null
    BigIntStatus (*divideTruncated)(const BigIntBackend* self, void* quotient, void* remainder, const void* a, const void* b) noexcept;
};
// The default every TemplateEnv starts with: the compiled-in backend, or nullptr
JINJA2CPP_EXPORT const BigIntBackend* GetBuiltinBigIntBackend() noexcept;
```

`TemplateEnv` (pimpl, `template_env.h:332`, so no layout change) gets
`SetBigIntBackend(const BigIntBackend*)` and `GetBigIntBackend() const noexcept`. The setter
validates `structSize`/`version`/non-null v1 fields/size and alignment caps, and reports failure by
0250's convention. `nullptr` means none.

Why the table is this small. pow, abs, floor division and modulo, rounding, base conversion and
int-to-float are composed in the core from these primitives, so every backend gets Python's
semantics from one implementation. Each primitive is something every bignum library has natively.
That is also why division is *truncating* in the contract: measured here, cpp_int gives
`-(2^64) / 7 = -2635249153387078802 r -2`, where Python floors to `-2635249153387078803 r 5`. Hash
is left out: mapping keys are decimal spellings (0013/0036, D13 b) and `unique` sorts and compares
(`src/filters.cpp:991`). The `self` parameter lets a backend carry state, such as an allocator (0252).

Boost adapter `include/jinja2cpp/big_int/boost_cpp_int.h`, header-only like `binding/*.h`:
`inline const BigIntBackend* BoostCppIntBackend()` over `boost::multiprecision::cpp_int` with
`BOOST_MP_STANDALONE`. Measured: `sizeof(cpp_int) == 32`, `alignof == 16`, so it fits the 64/16
slot, and small values (up to about 128 bits) need no allocation of their own. A TU including it
took 4.7 s at `-O2`.

### 3.3 Internal model (0270, 0271)

- `ValueKind::BigInt` (new enumerator, `src/internal_value.h:272-288`). A new kind, rather than
  reusing `Int`, means every `switch` on the kind fails `-Wswitch -Werror` until it handles big
  values (`expression_evaluator.cpp:467`, `filters.cpp:63,176`, `value_methods.cpp:2163`). It also
  means `InlineScalar`'s "scalar kinds" (`expression_evaluator.h:564`) cannot mistake a boxed value
  for an immediate one. `PythonTypeName` says `int`.
- `detail::BigIntHandle { const BigIntObject* object; }` is appended to `ValueStorage`
  (`internal_value.h:362-379`). `m_obj` owns the object, as for strings, and
  `Kind()` (`:1108-1118`) gets one entry.
- `BigIntObject final : ValueObject { const BigIntBackend* backend; uint32_t size; }` is followed in
  the same allocation by either the decimal characters (text form, `backend == nullptr`) or the
  backend's object (computed form). It is allocated once, as `StringObject::Make` is (`:829-858`).
  The destructor calls `backend->destroy`.
- Visitors see `BigIntView { const BigIntBackend* backend; const void* object; std::string_view decimal; }`,
  produced by `RecursiveUnwrapper` (`value_visitors.h:106`) both from `BigIntHandle` and from a
  public `BigInt` read in place through `ValueRef` (`ApplyUnwrapped`, `:180`).
  `BaseVisitor` (`:214`) gets a **deleted** `operator()(const BigIntView&)`, so a visitor that can
  meet one and does not say what it does fails to compile. The compiler becomes the audit, not the
  catch-all that returns an empty value.
- Invariant: no `BigIntObject` holds a value inside int64. Entry from the host
  (`Value2IntValue`, `internal_value.cpp:1801`, via `InputValueConvertor`, `value_visitors.h:518`),
  from literals and from every operation normalises through `toInt64`. Exit to the host
  (`IntValue2Value`, `internal_value.cpp:2083`) produces `BigInt` through `toDecimal`, so no box
  ever escapes a render.
- An operand in text form, or one from another backend, is converted with `fromDecimal` into a
  stack slot (at most 64 B) for the call, so it costs no heap allocation. A result costs one box
  allocation plus whatever the backend allocates.
- Limits: `MaxBigIntBits = 1 << 20` (128 KiB per value), next to `MaxSequenceSize`
  (`internal_value.h:393`). It is checked before multiplying (`bits(a) + bits(b)`) and before `**`
  (`bits(base) * exp`), which stops `2 ** 10 ** 10` before it allocates. Decimal conversion of more
  than 4300 digits raises ValueError, as CPython 3.11+ does: `{{ 2 ** 20000 }}` raises in Python
  while `{{ (2 ** 20000) % 7 }}` renders `4`, and the same holds here. A literal of more than 4300
  digits fails at load, as in Python.

### 3.4 Which backend a computation uses

At render entry `TemplateImpl::Render` already takes `RenderWorkspace::ForThisThread()`
(`src/template_impl.h:83`). It stores the env's backend pointer there, and a RAII guard restores the
previous one for nested renders. That costs two stores per render. Only the cold promotion path
reads it. Parsing needs no backend, because literals are made in text form. Per env, not per
process: two libraries in one process can use different backends, and there is no global state.

### 3.5 Build option, installed config, ABI statement

- `JINJA2CPP_WITH_BIGINT` (STRING `none|boost`, default `none`), next to
  `JINJA2CPP_WITH_JSON_BINDINGS` (`CMakeLists.txt:25-26`). There is no `custom` value: custom
  backends register at run time. `boost` compiles `src/big_int_builtin.cpp`, which includes the
  adapter header, with a private define. No public header changes.
- Installed: `set(jinja2cpp_BIGINT "@JINJA2CPP_WITH_BIGINT@")` beside `jinja2cpp_JSON_BINDINGS`
  (`cmake/public/jinja2cpp-config.cmake.in:9`), a `bigint=` variable in `jinja2cpp.pc.in`, and
  `GetBuiltinBigIntBackend()->name` at run time. Multiprecision is header-only and compiled in, so
  consumers get no `find_dependency`.
- ABI: `Value` layout is fixed by 0269 in every build. `BigIntBackend` grows at its end only, so
  appending is not an ABI break (api-2.0.md 5.4 gets one sentence on this). The 0076 abidiff job
  reports additions only.

## 4. Overflow promotion and the perf gate (0271)

Where 0015 detects overflow today (`src/value_visitors.h`): `+ - *` at `:1372-1395`, unary minus of
`INT64_MIN` at `:1012-1025`, `IntegerPow` at `:1305-1331` (either multiply), and `INT64_MIN // -1` in
`IntegerFloorDiv` at `:1334-1349`. `IntegerRemainder` (`:1353`) cannot overflow, and `PythonRoundInt`
(`filters.cpp:2153-2187`) also overflows.

The change: each `ThrowIntegerOverflow()` becomes `return detail::PromoteBinary(oper, a, b)` (or
`PromoteNegate` / `PromotePow(base, exp)`, restarting from the original operands). These are
`noinline, cold` functions in `src/big_int.cpp` (macros at `internal_value.h:186-190`). With no
arithmetic backend they throw, as today, with a message that names `JINJA2CPP_WITH_BIGINT`. The
non-overflow instructions do not change. `IsNumber`/`VisitNumber`/`ApplyToNumbers` (`:2020-2046`)
stay int/double/bool only, so the fast path never sees a big value. Big operands take the general
`Apply2` route.

- GCC/Clang (and clang-cl): `__builtin_*_overflow`, already used. MSVC `cl`: the portable fallbacks
  at `:952-997` stay. `_mul128`/`__mulh` would save the divisions in `MulOverflows`, but only with a
  Windows bench to show it, which the cloud cannot run, so it is not in scope. C23 `ckd_*` is not
  available at C++17.
- Demotion: every computed result goes through `toInt64`. `-(9223372036854775808)` becomes
  `INT64_MIN` as an int64.
- Gate (0137 Cachegrind, `bench/count.py`, repeatable to about 0.05%): every `Render/*` and `Load/*`
  case within ±0.5% Ir and identical allocation counts against the base, in the default **and** the
  boost build. 0271 adds `bench/cases/int_arith` (an int64-only loop of `+ * // % ** <` and `sum`,
  with its Python twin) so the gate watches exactly this code. Also reported, not gated: `.text`
  delta and build time. `Apply2` grows from 17x17 to 18x18 alternatives (+12% in the pair tables of
  `BinaryMathOperation`), and the Boost TU takes about 5 s in boost builds only.

## 5. Behaviour by build

| Situation | Default (`none`) | With a backend (boost or user) | Python |
|---|---|---|---|
| int64 arithmetic leaves int64 (`9223372036854775807 * 2`, `2 ** 100`, `(-2**63) // -1`) | OverflowError, as 0015 | exact | exact |
| Literal beyond int64 (`12345678901234567890`, `0x10000000000000000`) | text form: prints, compares and converts to float exactly; `+ 1` raises | exact | exact |
| **Change from today:** that literal is a float now | `{{ 12345678901234567890 + 1 }}` goes from `1.2345678901234567e+19` to OverflowError (loud, not silently wrong) | `12345678901234567891` | `12345678901234567891` |
| `Value(ULLONG_MAX)`, JSON uint64, host `BigInt` | renders `18446744073709551615`; arithmetic raises | exact | exact |
| `'99999999999999999999'\|int`, `1e20\|int`, any base | exact (core digit converter; fmt prints doubles exactly) | exact | exact |
| big `+ 1.5`, `> 1.5`, `\|float`, `/` | exact comparison; to-float via decimal, OverflowError past 2^1024 | same | same, except `/` (below) |
| `-x`, `abs`, `%d %x %o %e`, `{:,}`, tojson, pprint, `is number/integer/even/odd` | work (they only need the representation) | work | work |
| `//`, `%`, `**`, `round(x, -n)`, `sum`, `divisibleby` on big | OverflowError | exact | exact |
| `range` with big bounds | OverflowError (deliberate; new case) | same | lazy range |

Divergence kept: `big / x` divides as doubles (0015 already records this for int/int beyond 2^53).
Python raises only when the *quotient* overflows, so `2**1100 / 2**1000` raises here.

## 6. Every numeric path

"core" means composed by the library from the representation or the table; "op" names the
backend functions needed.

| Path | Site | Without backend | With backend (ops) |
|---|---|---|---|
| `+ - *` | `value_visitors.h:1372-1395` | raise on overflow | promote: fromInt64, add/subtract/multiply |
| unary `-`/`+`/`not` | `:1012-1025` (UnaryOperation) | `-INT64_MIN` raises; `-text` flips sign in core | negate |
| `//` `%` | `:1334-1369` | big operand raises | divideTruncated, then core floor fix-up (add/subtract, sign) |
| `**` | `:1305-1331` | raises | core square-and-multiply over multiply with bitLength guard; 0/±1 bases short-cut; negative exponent goes to float |
| `/` | `:1396-1401` | core to-float (bitLength ≤ 1024, then decimal + strtod) | same |
| `==` `<` ... int64 vs big | `:1402-1410` | sign only (invariant) | same |
| big vs big | new overloads | decimal compare in core | compare |
| int vs float | `CompareIntDouble` `:1185` | exact: float to exact decimal (fmt) and compare | same |
| sort, dictsort, groupby, unique, `in` | `filters.cpp:991`; `testers.cpp:342` | compare | compare |
| dict keys, hashability | `expression_evaluator.cpp:851-865`; `filters.cpp:962` | decimal spelling; hashable | same |
| truthiness | BooleanEvaluator `:1793` | true (non-zero by invariant) | same |
| str, `~`, print | ValueRenderer `:300-318`; `out_stream.cpp:217` | toDecimal into a stack buffer; > 4300 digits ValueError | same |
| tojson, pprint | `serialize_filters.cpp:405-415`, `:35/:159` | digits | same |
| `int` filter | `filters.cpp:2363`, `:2004-2060` (uint64 overflow currently fails) | text form, bases 2-36 via core digit converter | normalised |
| `float`, `filesizeformat` | `filters.cpp:2394`, `:2311` | core to-float; OverflowError | same |
| `abs` | `filters.cpp:2415` | core sign flip | negate |
| `round` | `filters.cpp:2153`, `:2454` | ndigits ≥ 0 unchanged; < 0 raises | core half-even over divideTruncated/multiply/add/compare |
| `sum` | `filters.cpp:940` | through `+` | promotes |
| `%d %i %u %x %X %o`, `%e %f %g`, `%c`, `*` width | `python_format.cpp:733-791`, `:570`, `:40` | core base conversion; to-float; `%c` and `*` raise OverflowError (Python) | same |
| `str.format` | `value_methods.cpp:1194` | decimal with grouping | same |
| int methods (`bit_length`) | `value_methods.cpp:2163` | core digit converter | bitLength |
| index-sized arguments (`IntArg`, NumberEvaluator) | `value_methods.cpp:127`; `value_visitors.h:1860` | OverflowError ("cannot fit 'int' into an index-sized integer") | same |
| `str * big`, `list * big` | `value_visitors.h:1505,1605,1682` | OverflowError (Python) | same |
| `list[big]` | `internal_value.cpp:389`; `value_methods.cpp:2264` | undefined (Python's IndexError becomes undefined) | same |
| slices with big bounds | `internal_value.cpp:619`, `:839` | clamp by sign (Python: `[1,2,3][2**64:]` gives `[]`) | same |
| `range` | `global_functions.cpp:102-139`; `internal_value.cpp:1515-1535` | OverflowError | same |
| literals | `lexer.cpp:44-77`, `lexer.h:214`; `expression_parser.cpp:986` (an attribute index stays int64 only) | text form | same |
| host to/from template | `internal_value.cpp:1801`, `:2083`; `value_visitors.h:518` | normalise; BigInt out | same |

## 7. What 0178 needs from 0250 and 0252

From 0250 (error contract):
- N1. An `ErrorCode` for Python's OverflowError at render. Today `ThrowIntegerOverflow` throws
  `std::runtime_error`, which becomes `UnexpectedException` (`template_impl.h:414-417`).
- N2. A code for Python's ValueError (the 4300-digit limit), or the generic Python-exception code if
  0250 adds one.
- N3. A resource-limit code for `MaxBigIntBits`, shared with 0097. *Settled by 0250 (docs/api-2.0.md
  §2.1, PR #584):* limits report `jinja2::Exception{ResourceLimitExceeded}`.
- N4. The checked-factory convention for `BigInt::Parse` (proposed: `std::optional`, since there is
  one failure reason) and for `SetBigIntBackend` validation.
- N5. The throw from host construction that 0250 lists (0172's `std::overflow_error`) goes away in
  0269. Tell the 0250 thread.
- N6. The backend boundary is noexcept with statuses: `DivisionByZero` maps to ZeroDivisionError,
  `OutOfMemory` to `std::bad_alloc` (0250: only for real system out-of-memory, never for a limit),
  `TooLarge` to N3 (`ResourceLimitExceeded`), and
  `Unsupported`/`BadInput` to an internal error naming `backend->name`.

From 0252 (memory contract):
- M1. The box (header plus at most 64 B of payload) is allocated by the core in one allocation on
  the same path as `StringObject`. Whatever 0252 decides for value objects applies to it.
- M2. Digits beyond the backend's own inline storage are the backend's to allocate. The contract's
  "what allocates" table should list "an int result outside int64: one box plus the backend's own
  allocations". The sample uses `std::allocator`. `self` lets a backend use an arena.
- M3. The only per-thread state is the current-backend pointer during a render. There is no cache.
- M4. `BigInt` holds a `std::string`, consistent with 0252's recommendation that `Value` stay on
  `std::allocator`.

## 8. Fit with 0230, CI, parity corpus, docs

- **0230:** no Boost include in `src/` except `src/big_int_builtin.cpp`, compiled only when
  `boost` is selected. When 0230 adds a check for this, that file is its one exemption. Internal
  deps: before 0230, add `multiprecision` to `BOOST_INCLUDE_LIBRARIES`
  (`thirdparty/thirdparty-internal.cmake:6`) only when the option is selected. After 0230, fetch
  boostorg/multiprecision and boostorg/config pinned with `URL_HASH` in
  `thirdparty/internal_deps.cmake`, and build with `BOOST_MP_STANDALONE` (docs/release-2.0.md
  section 6 checked that it needs only Config). External and conan modes locate the headers only.
  In `sbom/components.json`, Multiprecision and Config become an optional component
  (`selected_by: JINJA2CPP_WITH_BIGINT=boost`, BSL-1.0, 0226). Add lines to 0006 for a Conan option
  `bigint=[none, boost]` and a vcpkg feature `bigint-boost`.
- **CI** (pairwise, `linux-build.yml:37-50`): `bigint: boost` on rows `:57` (17 gcc-12 Release
  static, json boost, noderef OFF), `:62` (20 gcc-14 Release shared nlohmann), `:64` (20 clang-20
  Debug static nlohmann noderef OFF) and `:67` (23 clang-18 Release shared rapid noderef OFF).
  With those four, `boost` meets every standard, compiler, build type, linkage, binding and
  noderef value, and the eight `none` rows still do too (checked by hand; the comment block gets
  one sentence). Also: the sanitizer row `:185` (gcc-14 ASan+UBSan), one Windows row
  (`windows-build.yml:49`, C++20 Debug shared, which exercises the MSVC fallback and the DLL
  export), one macOS row, and the fuzz build (DoS through `**` and the size caps). The adapter
  header test also compiles in every row that has Boost headers (json=boost), whatever the option.
- **Parity, build-dependent divergences (0273):** a kind may carry a condition,
  `operators.int_overflow_mul rejects@nobigint 0178 <reason>`. When the suite's default env has no
  arithmetic backend, the case must diverge as listed. When it has one, the case must match. The
  suite records `# features: bigint` in `JINJA2CPP_PARITY_RESULTS`. `update_divergences.py` touches
  only lines whose condition holds in the recording build. `ParityRegistry` validates the suffix,
  and `generate.py --report` counts such lines as "matches with a backend". The steward's flow is
  unchanged, since it runs on the default build.
  Default build: `literals.int_big` and `filters.int_out_of_range` match, and their lines go in
  0270. `int_overflow_mul`, `int_big_pow`, `int_min_floordiv_overflow` and `round_int_overflow`
  become `@nobigint` in 0272. The parity `ToValue` (`test/parity/parity_test.cpp:261`) reads
  `number_unsigned` as `uint64_t`, so context values up to 2^64-1 test the host path.
- **Docs:** `docs/parity.md:120,149` change from "deliberate" to "build option", and
  `docs/release-2.0.md:135` likewise. README gets the build option plus a "Big integers" section
  (registering a backend, the one-file example in the adapter header). `docs/api-2.0.md` 3.1 rows
  and the 5.4 sentence. MIGRATION (0077) covers the new alternative, `Value(uint64_t)` no longer
  throwing, and literals beyond int64 now being ints. `docs/dependencies.md` gets the Boost row.

## 9. Phases (task numbers checked free)

Renumbered 2026-10-18 12:30 (first draft used 0253-0257): 0253-0255 and 0262-0268 belong to 0250
(#584), 0258 to 0183 (#579), 0259-0261 to 0076 (#580). Checked master and every remote branch:
0269-0273 are free and named nowhere else.

| Task | PR | Blocks G1? | Depends | Touches (for `task_batches.py`) |
|---|---|---|---|---|
| **0269** Public `BigInt` value | public type, `Value` alternative and accessors, `Value(uint64)`/Reflect/JSON bindings store BigInt; until 0270, a BigInt entering a template raises OverflowError | **yes** (layout of a user-allocated type) | #573 (0172), 0250 decision (N4), 0076 PR-2 if it has landed | `include/jinja2cpp/{big_int.h,value.h,reflected_value.h,binding/*.h}`, `src/value.cpp`, `src/internal_value.cpp#Value2IntValue`, `test/basic_tests.cpp`, `test/big_int_test.cpp`, `test/binding/*`, `docs/api-2.0.md` |
| **0273** Build-conditional parity divergences | `@<feature>` kinds, results header, script and report | no | none (parallel with 0269) | `test/parity/{parity_test.cpp,update_divergences.py,generate.py,README.md}` |
| **0270** Big integers in the core, text form | `ValueKind::BigInt`, box, `BigIntView`, deleted catch-all, every row of section 6 "without backend", literals, limits | no (internal), in 2.0 | 0269 | `src/internal_value.*`, `src/value_visitors.h`, `src/lexer.*`, `src/filters.cpp`, `src/python_format.cpp`, `src/serialize_filters.cpp`, `src/out_stream.cpp`, `src/testers.cpp`, `src/value_methods.cpp`, `src/expression_evaluator.*`, `src/global_functions.cpp`, `src/big_int.*` (new), `test/parity/divergences/{literals,filters}.txt`, `test/parity/cases/*.py` |
| **0271** Backend interface and promotion | `big_int_backend.h`, env methods, builtin getter, render-entry pointer, promotion, composed ops, test-only backend, `int_arith` bench | no (additive), in 2.0 by D2 | 0270 | `include/jinja2cpp/{big_int_backend.h,template_env.h}`, `src/template_env.cpp`, `src/template_impl.h`, `src/render_workspace.h`, `src/value_visitors.h#BinaryMathOperation`, `src/filters.cpp#PythonRoundInt`, `src/big_int.*`, `test/big_int_test.cpp`, `test/big_int_toy_backend.h`, `bench/cases/int_arith` |
| **0272** Boost backend and build option | adapter header, option, deps, config, SBOM, CI rows, `@nobigint` lines, docs | no (additive), in 2.0 | 0271, 0273; coordinate with 0230/0229 (`thirdparty/`, `sbom/`) | `include/jinja2cpp/big_int/boost_cpp_int.h`, `src/big_int_builtin.cpp`, `CMakeLists.txt`, `thirdparty/*`, `cmake/public/*`, `jinja2cpp.pc.in`, `sbom/*`, `.github/workflows/*`, `test/parity/divergences/{operators,filters}.txt`, `docs/{parity.md,dependencies.md}`, `README.md` |

0178 becomes the umbrella: `status: planned` with links to the five tasks. Its "Done when" needs
revising (Q5). Its line "Not a 2.0 condition" contradicts D2 and should read "0269 is a G1
condition; 0270-0273 are in 2.0 by D2 and are additive".

Tests that prove each step:
- **0269:** rows in `test/basic_tests.cpp` that replace #573's `EXPECT_THROW` rows (they encode the
  old behaviour): `Value(ULLONG_MAX).IsBigInt()`, `ToString` is `18446744073709551615`,
  `Reflect(uint64_t(-1))`, `Value(INT64_MAX)` stays `IsInt`. A `BigInt::Parse` table
  (`"007"` gives `"7"`, `"-0"` gives `"0"`, and `""`, `"-"`, `"+1"`, `"1e3"`, `" 1"` are rejected),
  comparison rows, the binding tests (uint64 max in all three), `Visit` over a BigInt, one row
  showing that a template reading one raises (flipped by 0270), and a `fwd_test` name. abidiff
  report: `sizeof(Value)` unchanged.
- **0273:** `ParityRegistry` rows for a valid suffix, an unknown suffix and a missing kind, plus a
  self-check of `update_divergences.py` on a fixture results file with and without the feature
  header.
- **0270:** a new `BigIntNoBackend` table in `test/big_int_test.cpp` (`MULTISTR_TEST`, narrow and
  wide), one row per "without backend" cell of section 6, outputs taken from Python. Parity: the
  `literals.int_big` and `filters.int_out_of_range` lines are deleted. New corpus cases:
  `literals.int_big_hex`, `literals.int_min_literal` (`-9223372036854775808`),
  `literals.int_digits_limit`, `operators.int_big_compare_float`, `operators.int_big_true_div`,
  `operators.int_big_repeat`, `filters.big_int_print` (string/tojson/pprint/abs/float/filesizeformat),
  `filters.format_big_int` (`%d|%x|%o|%e` and `{:,}`), `filters.sort_unique_big`,
  `subscripts.big_index_and_slice`, `globals.range_big_bounds`, `tests.big_int_tests`, and a context
  case with `18446744073709551615`. Cases that need arithmetic get `@nobigint` lines owned by 0178.
  Also the sanitizer configuration, and the perf gate on the default build.
- **0271:** the same table, run as `BigIntToyBackend` with `test/big_int_toy_backend.h`, a ~200-line
  decimal-string backend on all platforms MSVC included, so the interface is tested in every CI row
  with no Boost. Rows give Python's outputs for `//`/`%` signs (`-(2**64) // 7` is
  `-2635249153387078803`, `% 7` is `5`), `**`, round, sum, mixed float, and demotion. Interface rows:
  `structSize` too small, version 2, `objectSize` 65, null field (all rejected); `OutOfMemory`
  becomes `bad_alloc`; division by zero; two backends mixed; `2 ** 2 ** 21` hits the cap; `str` of
  `2 ** 20000` raises ValueError. Gate: section 4.
- **0272:** in boost rows, `BigIntBoost` runs the 0271 table against `BoostCppIntBackend()` and
  against the built-in default. The parity lines become `@nobigint` and must match in boost rows. The
  `test/package` consumer checks `jinja2cpp_BIGINT` and `GetBuiltinBigIntBackend()->name`.
  `scripts/sbom.py --check` passes. The perf gate runs on the boost build.

## 10. What this makes hard next

- Every new numeric site has to handle `ValueKind::BigInt`. `-Wswitch` and the deleted
  `BaseVisitor` overload make that a compile error rather than a silent empty, but it is a
  permanent tax on contributors.
- `BigIntBackend` v1 semantics are frozen: truncating division, the 64 B/16 B slot, decimal I/O.
  Adding ops (bitwise operators, gcd, a faster to-float) means appending fields and bumping
  `version`, and the core must keep a composed fallback for v1 backends.
- The text form will tempt someone to grow it into a bignum ("just add `+`"). Keep the line at
  representation; exact arithmetic is a backend's job.
- 0036's full key type: big keys hash through their decimal spelling. That is correct, but slow
  for big-keyed dicts (rare).
- Values from two backends in one render go through decimal. That is correct but slow, and only
  happens if a host mixes envs.
- Python's `/` on big ints is correctly rounded; ours divides doubles, and closing that needs a
  scaled-division routine in the core.
- Made easier: 0248 (Python drop-in) can register a backend over CPython's `PyLong` and get exact
  Python ints with no Boost, and the WASM playground (0244) stays Boost-free.

## 11. Questions for Ruslan, decided

**Decided by Ruslan 2026-10-18 14:31 ("go recommended"): every question below as recommended.**
#573 merged first (train #583); 0269 replaces its throw before the tag. 0250 phase 0253
(`Exception{IntegerOutOfRange}` for that throw) and 0269 both edit `detail::ToStoredInt`: whichever
lands second rebases onto the other.

- Q1. A runtime table registered per `TemplateEnv`, with a compile-time choice of the built-in
  default, rather than a pure compile-time policy. **Recommend: yes.** The cost is a larger 2.0
  surface (one versioned struct, two env methods, one free function). The gain is user backends on
  prebuilt binaries and one binary for every choice.
- Q2. Text-form big integers in the default build: literals and host values beyond int64 print and
  compare exactly, and arithmetic raises. This changes `{{ 12345678901234567890 + 1 }}` from a float
  to OverflowError. **Recommend: yes** (D1's "render or fail loudly" applied to literals too). The
  alternative is strict: with no backend, a BigInt entering a template raises, literals stay floats,
  and `literals.int_big` stays a divergence.
- Q3. 0172's constructor throw is replaced in 2.0 by a stored `BigInt`. **Recommend: merge #573
  now for G2, and let 0269 upgrade it before the tag**, so no release ever throws.
- Q4. G1 waits for 0269 only. 0270-0273 are in 2.0 by D2 but, being additive, do not hold the freeze
  and would ship in 2.1 with no SOVERSION bump if late. **Recommend: yes.**
- Q5. Revise 0178's "Done when" to: "the six cases match in builds with a backend; in the default
  build `int_big` and `int_out_of_range` match and the other four are `@nobigint`; docs/parity.md
  calls integers a build option". **Recommend: yes.**
- Q6. Limits: `MaxBigIntBits = 2^20`, an internal constant, plus CPython's 4300-digit decimal limit.
  **Recommend: yes**; make it a 0097 setting only if someone asks.
- Q7. Names: `JINJA2CPP_WITH_BIGINT=none|boost`, `jinja2::BigInt`, `jinja2::BigIntBackend`, a public
  adapter header `jinja2cpp/big_int/boost_cpp_int.h`, and Boost.Multiprecision fetched standalone
  under 0230. **Recommend: yes.**
