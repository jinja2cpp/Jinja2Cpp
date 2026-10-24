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
