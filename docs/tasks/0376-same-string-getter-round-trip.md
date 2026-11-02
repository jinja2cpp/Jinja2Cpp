---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/735
priority: low
area: perf
touches: [src/value_visitors.h#SameStringGetter]
shares: [src/string_converter_filter.cpp, src/value_methods.cpp]
---
# GetAsSameString moves its string through the PLT

**Problem.** #662 (0314) added +0.35% to +0.51% Render instructions on `chat_llama`,
`chat_mistral`, `chat_qwen` and `html_autoescape`. These are trend steps on 27b0223. The
PR body reports the gate at +0.00% on every row.

A callgrind diff of 994d6e8 against 27b0223 puts most of the step on one new call:
libstdc++'s `std::string(std::string&&)`, reached through the PLT.
- It had 0 calls on `chat_llama` before #662, and has 639 a render after.
- On master 32564b3 it is about 630 calls a render on each chat case, 760 on
  `html_autoescape` and 570 on `config_file`.
- The calls come from `Apply<visitors::SameStringGetter<char>>`.

`SameStringGetter` returns `nonstd::expected<void, std::basic_string<CharT>>` and puts
the string in the error slot, through `MakeUnexpected`. `GetAsSameString` then copies
`result.error()` (an lvalue) into a `std::optional<std::basic_string<CharT>>`. Each
string argument therefore costs one move through the PLT plus one copy. The string
filters #662 rewrote read their string arguments this way.

0330 lists the same PLT signature in `RenderAsString`, fixed in 6fd648b. This is a new
place.

Found by the 2026-11-02 perf scout (task 0123).

**Proposal.**
- Make `SameStringGetter` return `std::optional<std::basic_string<CharT>>` (or a view
  when the value already holds a `ResultString`), so that `GetAsSameString` returns it
  without the `expected` round trip.
- At minimum, `GetAsSameString` should move out of `result.error()`.
- Check the #662 call sites in `string_converter_filter.cpp` and `value_methods.cpp`.
  Some may need only a `string_view` (`trim`'s chars, `replace`'s old and new).

**Done when.**
- `count.py` on `Render/chat_llama`, `chat_mistral`, `chat_qwen` and `html_autoescape`
  recovers at least 0.3% against the master base.
- Callgrind on `Render/chat_llama` shows no call to libstdc++'s
  `basic_string(basic_string&&)` from a `SameStringGetter` frame.
- No row gets slower by more than 0.1%, and the parity suite is unchanged.
