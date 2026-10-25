---
status: done
priority: high
area: api
depends: []
touches: [include/jinja2cpp/user_callable.h, test/user_callable_test.cpp]
---
# A typed `std::string_view` parameter given the other character width dangles

**Problem.** Found by the 0277 verifier under ASan. The same failure happens on master
(631c9ed9). Take a callable made by `MakeCallable` from a function pointer, which uses the typed
path through `ArgInfoT`, with a `std::string_view` (or `std::wstring_view`) parameter. When it is
called with a string of the other width (a `std::wstring` value for a `std::string_view`
parameter), `TypedParamUnwrapper` runs `param->SetValue(Promote(v))`. `ArgPromoter`'s
`operator other_string_view()` converts the text into `m_convertedStr`, which is a member of the
temporary promoter, and returns a view of it. The temporary dies at the end of `SetValue`'s
full-expression, so the stored view dangles by the time the function runs. ASan reports
stack-use-after-return. A template reaches this whenever host data puts a string of the other
width in front of such a callable. That is a host-API lifetime bug (REVIEW.md lifetime class).

**Proposal.** Give `TypedParam` owned storage for a converted string, and point the view at
that storage. Alternatively, do not take the promoting path for view parameters: keep a
`std::optional<std::basic_string<OtherChar>>` next to the `TypedParam` that the tuple holds
until `std::apply` returns. Check the lambda path (`UCInvoker`), where the promoter lives for the
whole call, and confirm it stays safe.

**Done when** a function-pointer callable with `std::string_view` and with `std::wstring_view`
parameters, given narrow and wide strings, Markup and WMarkup, passes under ASan, with rows in
`test/user_callable_test.cpp`.

**Resolution.** `TypedParam` of a `std::basic_string_view` owns an optional converted string.
`TypedParamUnwrapper` converts text of the other width (a string or a string view; a Markup
reaches it as its string) into that storage instead of going through `Promote`, and `GetValue`
makes the view from the storage and returns it by value, so the view survives the move of the
param into the argument tuple, which lives until `std::apply` returns. The lambda path
(`UCInvoker`) was already safe: its promoters are temporaries of the full-expression that calls
the function. Rows: `StringViewPointerParamConvert` (by-value and `const&` view parameters,
narrow and wide, short and long strings, Markup and WMarkup).

Fixed on the way, found by the verifier:
- A callable that returns a `std::string_view` or `std::wstring_view` (often a view of its
  argument) put the view in the result `Value`, which outlived the argument. Both paths now copy a
  returned view into an owned string (`MakeCallResult`). Rows: `StringViewResultConvert`.
- A lambda taking a view by value, given a string the host passed in, rendered None: the
  promoter of a view had both `operator string_view` and `operator const string_view&`, which
  made the conversion ambiguous, and `FuncTester` then skipped the call. The by-value operator is
  gone.
- A typed `Markup` parameter given text of the other width got no value. It now converts the
  text: a WMarkup stays a Markup, a wide string is escaped (rows in `MarkupTest`).

Filed: 0320 (an omitted argument behaves differently on the two `MakeCallable` paths).
