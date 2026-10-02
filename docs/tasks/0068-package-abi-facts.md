---
status: open
priority: high
area: build
depends: []
touches: [CMakeLists.txt, cmake/public/]
---
# The installed package does not carry the library's ABI choices

**Problem.** Two choices made when the library is built decide its ABI, and the installed
CMake package passes neither on to the consumer (found in the 0056 API survey,
`docs/api-2.0.md`):

1. **Vocabulary types.** `optional-lite`, `variant-lite`, `string-view-lite` and
   `expected-lite` resolve to the `std::` types when the translation unit's standard has
   them. The library is built at C++14 by default; a consumer compiling at C++17 sees
   `jinja2::Value::ValueData` as `std::variant<...>` while the library was compiled with
   `nonstd::variants::variant<...>` (checked: same `sizeof`, different types). Every
   function whose signature or layout involves these types (`Value`, `Result<T>`,
   `IFilesystemHandler::GetLastModificationDate`, `IListItemAccessor::CreateEnumerator`, ...)
   is an ODR violation: link errors at best, wrong vtable calls at worst. Only
   `variant_CONFIG_SELECT_VARIANT` is pinned, and only for MSVC. With `expected-lite`, the
   same happens between a C++17 library and a C++23 consumer.
2. **Link type.** `cmake/public/jinja2cpp-config.cmake.in` always declares
   `add_library(jinja2cpp STATIC IMPORTED)`, sets `PUBLIC` compile definitions on an imported
   target (only `INTERFACE` is valid there), keys `JINJA2CPP_LINK_AS_SHARED` on the
   *consumer's* `JINJA2CPP_BUILD_SHARED` variable, and exports no namespaced target. It is
   hand-written because of an old CMake bug (#14444, comment in `CMakeLists.txt`).

**Proposal.**
- Add `optional_CONFIG_SELECT_OPTIONAL`, `variant_CONFIG_SELECT_VARIANT`,
  `nssv_CONFIG_SELECT_STRING_VIEW` and `expected_CONFIG_SELECT_EXPECTED` as `PUBLIC` compile
  definitions of the library target, set to what the build's standard selects, so the
  build tree and the installed package both force it on consumers. A consumer at a lower
  standard than a `std`-selecting library then fails to compile with a clear message
  instead of linking wrongly.
- Replace the hand-written config with `install(EXPORT ... NAMESPACE jinja2cpp::)` (CMake
  ≥ 3.23 is required already) plus the existing dependency lookup; keep a `jinja2cpp`
  alias for old consumers.
- A CI job that installs a C++14 build and builds a small C++17 consumer against it.

**Done when.** That job is green and fails when the definitions are removed.

**Next.** 2.0 adds the selection to the inline ABI namespace as a second guard (0056).
If 0008 raises the floor to C++17, only `expected` remains to pin.
