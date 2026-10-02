---
status: open
priority: low
area: parity
depends: [0019]
touches: [src/string_converter_filter.cpp, src/filters.cpp#ValueConverter]
shares: [src/binding/, CMakeLists.txt]
---
# Filter behaviour: what 0019 left behind

**Problem.** Task 0019 ported the builtin filters to Jinja2's semantics, but some gaps
stay, each smaller than the filter it lives in:

1. **Unicode case mapping.** `upper`, `lower`, `capitalize` and `title` change ASCII letters
   only (`std::toupper` on single code units); Python maps every cased character, so
   `'éa'|upper` is `ÉA`. Case: `filters.case_non_ascii`.
2. **HTML entities in `striptags`.** markupsafe ends `striptags` with `html.unescape`,
   which knows the whole HTML5 table (2231 names, some valid without `;`, like `&amp`).
   `HtmlUnescape` in `src/string_converter_filter.cpp` decodes numeric references and 18
   common names. Case: `filters.striptags_named_entities`.
3. **Integers beyond int64.** `int` gives the default for `'99999999999999999999'|int` and
   `1e20|int`; Python returns the big integer. The value model has no big integers, so
   this probably stays a documented limit (decide with 0015's overflow rules). Case:
   `filters.int_out_of_range`.
4. **Dead JSON serializers.** `tojson` now has its own writer (`PythonJsonWriter` in
   `src/serialize_filters.cpp`, Python's `json.dumps` layout for every JSON binding), so
   `ToJson` in `src/binding/*_json_serializer.{h,cpp}` has no caller. Remove the three
   serializers, their CMake lines and their tests, or keep them as a public helper.
5. **Rounding beyond int64 and double edge cases.** `9223372036854775807|round(-1)` wraps
   (`PythonRoundInt`); `round(ndigits)` with `ndigits < -18` returns 0 for ints where
   Python gives a multiple of 10**19; `round(300, 'floor')` and other huge precisions can be
   off by an ulp; `1.797e308|round(-308)` gives `inf` where Python raises OverflowError.
6. **`%d` of a large float.** `'%d'|format(1e30)` errors ("cannot convert float infinity"),
   Python prints the exact integer. Same root as item 3.
7. **`%a` is `%r`.** `'%a'|format('é')` gives `'é'`; Python ASCII-escapes it to `'\xe9'`.
8. **`format` without `%`.** A format string with no `%` still goes through fmt's `{}`
   syntax (kept for existing templates), so `'x'|format(1)` renders `x` where Python raises
   "not all arguments converted". Decide whether to keep the fallback.
9. **`int`/`float` on Unicode digits and spaces.** Python's `int()`/`float()` accept
   non-ASCII digits (`'٤٢'`, `'１２'`) and Unicode whitespace around the number; ours give
   the default.
10. **Wrong type accepted instead of TypeError.** `nope|tojson`, `tojson(1.5)`,
    `None|batch(2)`, `5|batch(2)` and `undefined|slice(3)` render something; Python
    raises (or, for the last, yields three empty columns). A dotted attribute path whose
    parent is missing (`map(attribute='addr.zip')` without `addr`) gives undefined where
    Python raises UndefinedError.
11. **Wide templates.** In `TemplateW`, non-ASCII text through `tojson`, `format`,
    `urlencode` and `xmlattr` is lost, because the narrow/wide conversion goes through the C
    locale (task 0035, lines `wide.filters.*`). The old wide `urlencode` emitted code
    points as raw bytes (`%E9` for `é`), not UTF-8, so it was never right either.

Also a deliberate divergence, not a bug: `attr` reads the keys of a user-provided map
(`GenericMap`: reflected structs, but also JSON objects from the bindings) as attributes,
because reflected fields are attributes. Python would not see a JSON object's keys.

**Done when.** No line of `test/parity/divergences/` names task 0048, item 4 is decided,
and `ctest` passes.
