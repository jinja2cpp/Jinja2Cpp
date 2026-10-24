---
status: open
priority: medium
area: build
touches: [src/, thirdparty/, CMakeLists.txt, cmake/public/, sbom/components.json]
---
# A Boost-free core

**Problem.** Boost is the only heavyweight dependency: internal mode configures 51 Boost
libraries and installs 56 MB of headers plus four static libraries next to
`libjinja2cpp`, and static consumers must link them. The code uses nine of them, each for
a small utility C++17 lacks (inventory and numbers in docs/dependencies.md, 0187 audit):
`span`, `small_vector`, `intrusive_ptr`, `unique_any`, `iterator_facade`, `filesystem`,
three string algorithms, `predef` endian, and Boost.JSON as the default `{% meta %}`
parser.

**Proposal.** Needs an architect plan (public build interface and perf-sensitive
containers). Order by payoff and risk:
1. `std::filesystem` for `RealFileSystem` (drops `boost_filesystem` and `boost_atomic`).
2. nlohmann (header-only) as the default JSON backend, or a small internal metadata
   reader; Boost.JSON stays selectable (drops `boost_json` and `boost_container`).
3. String algorithms, endian, `iterator_facade`, `unique_any`: local replacements.
4. `span`, `intrusive_ptr`: small internal headers.
5. `small_vector`: an internal one, gated by the bench/ Cachegrind check.
Then Boost becomes optional (only for `JINJA2CPP_WITH_JSON_BINDINGS=boost`) and the deps
modes, installed config and pkg-config file stop asking for it.

**Decision (Ruslan, 2026-10-24).** Step 2 uses a small internal reader, not nlohmann,
simdjson or Boost.JSON. The reader builds `Value` directly and allocates through our
containers, so it follows the 2.0 memory contract. nlohmann's destructor terminates on OOM
(0279). The reader also applies `Settings::Limits` and the error-contract location rules to
`{% meta %}` text, which may be untrusted, and it lets `unique_any` go. Its parse of
untrusted input needs a fuzz target (0267) and JSONTestSuite conformance cases. The opt-in
binding headers stay as they are; simdjson gets one too (0302).

**Done when.** The default configuration builds and installs with no Boost at all,
render/load benchmarks show no regression beyond noise, and sbom/components.json lists
Boost only as the optional Boost.JSON backend.

**Progress.** Step D1 (backlog proposal 2026-10-24, section A): `src/span.h`,
`src/small_vector.h` and `src/intrusive_ptr.h` name `boost::span`,
`boost::container::small_vector` (with `SmallVectorBase` and the doubling growth option)
and `boost::intrusive_ptr` once, as `Span`, `SmallVector` and `IntrusivePtr` in namespace
`jinja2`, and every call site uses the aliases. Steps D4 and D5 replace what these headers
point at. Step D2: `RealFileSystem` uses `std::filesystem`, so internal mode no longer
builds `boost_filesystem` and `boost_atomic` (43 Boost libraries configured instead of 47,
two static libraries installed instead of four), and the external and Conan modes no longer
ask for the filesystem component. C++17 cannot convert a `file_time_type`, and its clock's
epoch differs per standard library, so the modification date subtracts the clocks' current
difference rounded to whole seconds (the epoch offsets are whole seconds) and keeps whole
seconds, as Boost's `time_t` and `ConfinedFileSystem` do.

Step D3: the string algorithms, `predef` endian, `iterator_facade` and `unique_any` are
gone from `src/`. `ListRef::Iterator` defines its own forward-iterator members, the
`{% meta %}` parse result is kept in a `std::shared_ptr<void>`, the RapidJSON binding reads
`__BYTE_ORDER__` (MSVC targets are little-endian), and `wordcount` classifies through the
global locale's `ctype` facet as `boost::algorithm::is_alnum` did. A whitespace-only
`{% meta %}` block is now tested against ASCII whitespace (`\t\n\v\f\r` and space, what the
classic locale gave for `char`). `replace` with a count no longer searches its own
replacement again (`'aaa'|replace('a', 'ba', 2)` gave `bbaaa`, Python gives `babaa`) and
stops at the last occurrence: a count of 10**12 used to loop until killed, and
`'a'|replace('a', 'ba', 10**12)` grew without bound. Corpus cases
`filters.replace_count_not_rescanned` and `filters.replace_huge_count`. The Boost component
lists in CMake (`algorithm any`, internal `BOOST_INCLUDE_LIBRARIES`) still name the two
libraries; D7 rewrites them with the deps modes, since D2 edits the same lines.

Step D4: `src/span.h` and `src/intrusive_ptr.h` define `Span` and `IntrusivePtr`
themselves. `Span` is the dynamic-extent part of `std::span` (no call site used a fixed
extent) with element access, `first`, `last` and `subspan` checked by assertions outside
`NDEBUG`; `IntrusivePtr` calls the same `intrusive_ptr_add_ref`/`intrusive_ptr_release`
hooks. Unit tests in `test/support_types_test.cpp`. Swap and move use plain assignments
rather than `std::swap`/`std::exchange`: with those, GCC inlined less around moves of
`InternalValue` and allocation-heavy renders ran up to 0.5% more instructions; written as
Boost writes them, every benchmark is within 0.15% of master.

Step D5: `src/small_vector.h` defines `SmallVector`, `SmallVectorBase` and
`SmallVectorDoubling` itself, with Boost's layout (pointer, size and capacity, then the
inline buffer at the next offset aligned for `T`, which is how the base tells it from a heap
buffer) and growth (1.6x, or 2x with `SmallVectorDoubling`). Growth keeps the strong
guarantee and is safe against an argument that names an element; `shrink_to_fit` moves the
elements back inline when they fit. The growth options are part of `SmallVectorBase`'s
type, as in Boost. Two things the instruction counts asked for: moving a vector whose
elements are inline moves them with one `uninitialized_move` (the general move assignment
made parse up to 0.9% slower), and the growth paths are not inlined (inlined, they used up
GCC's inlining budget in `statements.cpp`, and `InternalValue`'s copy assignment went out of
line in the loop body). Against D4: Load -0.27% on average, every benchmark within +0.2%.
