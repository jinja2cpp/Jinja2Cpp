# C++-only filters

Jinja2C++ adds a few filters that Python Jinja2 does not have. A template that uses them
does not load in Jinja2 (`No filter named ...`). Since 2.0 each has the output described
here (docs/tasks/0233); before, `camelize`, `underscorize`, `escapecpp`, `toxml` and
`toyaml` were registered but rendered an empty string.

| Filter | Output |
|---|---|
| `applymacro(..., macro=name)` | The value passed to the macro `name`, with the other arguments; `macro` is a keyword argument, written after the positional ones |
| `camelize(upper_first=true)` | An identifier in CamelCase |
| `underscorize` | An identifier in snake_case |
| `escapecpp` | The text escaped for a C++ string literal |
| `toyaml` | The value as a block-style YAML document |
| `neutralize(specials=none, tags=none, separator=none)` | The text with every special token and tag of an LLM prompt broken by a separator |
| `fence(lang="")` | The text in a Markdown code fence its content cannot close |
| `count_tokens` | The number of tokens in the text, by the environment's tokenizer |
| `truncate_tokens(n, end="...")` | The text cut to at most `n` tokens |
| `pack(budget, attribute=none, by=none, reverse=false, reserve=0, stop=false)` | The items of a list whose text fits in a token budget, in their original order |

The five prompt filters and the prompt escaping mode are specified at the end of this file
(docs/tasks/0358, 0359; decided by Ruslan 2026-11-05, docs/api-2.0.md 3.3) and implemented.

`toxml` was removed in 2.0: JSON-like data has no single XML mapping, so it would be a format
of our own to define and support (docs/release-2.0.md, D16). A template that uses it fails
to load with `ErrorCode::UnknownFilter`; `xmlattr` covers attribute lists, and a macro
can write any XML shape.

## `camelize` and `underscorize`

Both take the value's `str()` and split it into words: runs of letters and digits (Unicode
`\w` without `_`), split again before an uppercase letter that follows a lowercase letter
or a digit (`fooBar`, `Vector3D`) and before the last of a run of uppercase letters that a
lowercase one follows (`HTTPServer` is `HTTP` and `Server`). Everything between words
(`_`, `-`, spaces, punctuation) is dropped.

- `camelize` gives each word a titlecase first letter and lowercase others and joins them:
  `device_type`, `device-type` and `DEVICE_TYPE` are `DeviceType`, `HTTPServer` is
  `HttpServer`. With `upper_first=false` the first word is all lowercase: `deviceType`,
  `httpServer`.
- `underscorize` joins the words in lowercase with `_`: `DeviceType` is `device_type`,
  `getHTTPResponseCode` is `get_http_response_code`, `__init__` is `init`.

Case mapping is Unicode's, as in `upper`/`lower`: `straße_name|camelize` is `StraßeName`.

## `escapecpp`

The value's `str()` as the contents of a C++ string literal, without the quotes: `\` and
`"` get a backslash, `\n`, `\r`, `\t`, `\a`, `\b`, `\f` and `\v` their escapes, and other
control characters (below U+0020, and U+007F) three octal digits (`\001`), which cannot run
into a digit that follows the way `\x` does. Other characters, `'` and non-ASCII ones
included, are kept, so a UTF-8 template gives a literal for a UTF-8 source file.

```
const char* text = "{{ message|escapecpp }}";
```

## `toyaml`

A YAML document that YAML 1.1 and 1.2 loaders (PyYAML, yaml-cpp, ...) read back as the same
data, with no trailing newline:

- `none` is `null`, booleans `true`/`false`, integers in decimal,
  floats in Python's shortest form with a `.` always present (`1.0e+20`), `.nan`, `.inf`
  and `-.inf`.
- A mapping is `key: value` lines in the mapping's order; a list is `- item` lines; a
  `dictsort` or `items` pair is a two-item list. Nested collections are indented by two
  spaces, a collection inside a list starts on the item's line, and an empty one is `[]`
  or `{}`.
- A string (a key too) is written plain only when it cannot be read as anything else: it
  starts with a letter, `_`, `/` or a printable non-ASCII character, holds only those,
  digits, spaces and `-.()+`, does not end with a space and is not a null or boolean word
  (`y`, `n`, `yes`, `no`, `on`, `off`, `true`, `false`, `null`, in any case). Any other
  string is double-quoted with YAML escapes (`\"`, `\\`, `\n`, `\t`, `\xHH`, `\uHHHH`, ...).
- A key longer than 255 bytes is written as an explicit `? key` with its value on a `:`
  line below, since YAML limits an implicit key to 1024 characters.
- Undefined values, `namespace()`, `cycler()`, `joiner()` and macros raise an error
  ("Object of type Undefined is not YAML serializable"), as `tojson` does.

```
{{ {'name': 'demo', 'ports': [80, 443], 'tls': {'enabled': true}}|toyaml }}
```

```yaml
name: demo
ports:
  - 80
  - 443
tls:
  enabled: true
```

The result is an ordinary string: under autoescape it is HTML-escaped like any other.

## Prompt filters

Templates that build prompts for a language model put untrusted text (user messages, retrieved
documents, tool output) into the channel that carries the instructions. What breaks out of a
prompt is a string the model or its tokenizer trusts: a special token (`<|im_start|>`,
`<|eot_id|>`, `[INST]`) or a closing tag or fence. These filters, the `Tokenizer` callback and
the prompt escaping mode are set up on the environment (`jinja2cpp/prompt.h`):

```cpp
auto& s = env.GetSettings();
s.autoescape = true;
s.autoescapeMode = jinja2::AutoescapeMode::Prompt;
s.promptEscaper = jinja2::PromptEscaper({ { "<|im_start|>", "<|im_end|>" }, { "document" } });
s.tokenizer = jinja2::Tokenizer([&tok](std::string_view text) { return tok.Count(text); });
```

The library keeps no list of a model's special tokens: the host reads them from its tokenizer
(the added tokens marked `special`) and registers them once.

### `neutralize`

`neutralize(specials=none, tags=none, separator=none)` takes the value's `str()`, Markup
included (it is an explicit request, like `forceescape`). Each argument defaults to the
environment's `PromptEscaper`; one that is given replaces it for that call. The output is part
of the 2.0 contract:

1. **Patterns.** Each special token, matched exactly and also ASCII case-insensitively. Each tag
   name `t` gives `<t` and `</t` followed by a tag boundary (`>`, `/`, a space, tab, CR, LF or the
   end of the text), ASCII case-insensitive. Empty specials are ignored.
2. **Inside.** At every position where a pattern starts, overlapping matches included, the
   separator is inserted after the first code point of the match (a whole UTF-8 sequence, or a
   whole surrogate pair in a 16-bit `wchar_t`). `<|im_start|>x` becomes `< |im_start|>x`.
3. **Edges.** If the text starts with a non-empty proper suffix of a special, the separator is
   prepended; if it ends with a non-empty proper prefix of a special or tag pattern, it is
   appended. So `'a<|im_'|neutralize ~ 'start|>'|neutralize` is `a<|im_  start|>`: two values
   joined cannot form a special.
4. **Separator.** Default a space. It must be non-empty and share no character (ASCII case
   ignored) with any pattern, a special must be at least two code points (one cannot be broken
   by an insertion), patterns and separator must be well-formed UTF-8, and no special or tag
   name may be longer than 256 bytes; anything
   else is an argument error (and `PromptEscaper`'s constructor throws `std::invalid_argument`).
   A tag name followed in the output by a separator counts as followed by a boundary, so a
   special that starts with a tag name's last letter cannot turn `<ab` into the tag `<a b`.

A zero-width separator (`separator='\u2060'`) is accepted, but it is a control character (Unicode
category Cf) that BERT-style text cleaning and many text pipelines delete, which puts the special
token back together. A visible space survives every normalizer.

The result is Markup when the input was Markup, or when the template autoescapes in prompt mode;
a plain string otherwise.

**Guarantee.** In prompt mode, a special token in the output lies inside trusted text (the
template's own text and literals, Markup values), or begins and ends in trusted text. The one
case the escaper cannot see is a template that writes a fragment around a value,
`<|{{ role }}|>` with `role = "im_start"`, as `<scr{{ x }}` in HTML.

### `fence`

`fence(lang="")` wraps the text in a Markdown code fence of `n` backticks, `n` being 3 or one
more than the longest run of backticks in the text, so the content cannot close it: `n`
backticks, `lang`, a newline, the content, a newline unless the content ends with one, `n`
backticks. `lang` must match `[A-Za-z0-9_+.#-]*`; anything else (a backtick, a newline) is an
argument error. The content is escaped first with the active escaper, so a fence never skips
escaping: neutralized in prompt mode and with autoescape off (with the environment's specials;
none means unchanged), HTML-escaped in HTML mode; Markup content is not escaped again. The
result is Markup when autoescape is on, a plain string otherwise. To cut a long text, truncate
before fencing: `doc|truncate_tokens(500)|fence`.

### Prompt escaping mode

`Settings::autoescapeMode` selects what autoescape escapes for: `Html` (the default, Jinja2's)
or `Prompt`. With `Prompt` and autoescape on, every `{{ }}` that is not Markup is neutralized
with the environment's `PromptEscaper`, and Markup means "prompt-safe". `|safe`,
`is escaped`, `{% autoescape %}` and Markup values work as in HTML mode. In prompt mode:

- String literals of the template are prompt-safe, so `'<|im_start|>' ~ role` keeps its special
  token and breaks one inside `role`; chat templates written for transformers work unchanged,
  with two Markup effects: printing a container or repr of literals shows `Markup('a')`, and a
  literal format string rejects `%x`, `%o`, `%c` and `%*d` as markupsafe does.
  Host values that carry special tokens on purpose (`bos_token`) are passed as `jinja2::Markup`.
- `|e`, `|escape` and `|forceescape` neutralize instead of writing HTML entities, and the
  results of `urlize`, `xmlattr` and `tojson` are neutralized (`tojson` always, as its result is
  always Markup; the other two when autoescape is on, when theirs is).
- Literals are Markup whether autoescape is on or off, so the Markup rules apply to them as to
  `|safe` text: `'<|im_start|>' + x`, `'%s' % x`, `'{}'.format(x)` and `', '.join(xs)` neutralize
  `x` even with autoescape off; `~` does under autoescape, also where Jinja2 joins as `str`
  (constant operands, volatile `{% autoescape %}` code), so that a run of literals stays Markup.
- `replace` on a value that is not Markup is a `str` replace, escaped on output, even when an
  argument is a literal: deleting characters from neutralized text could put a special back
  together (`content|replace('\n', '')`). On Markup it is Markup's replace, as in HTML mode.
- The precision of `%s` cuts the neutralized text, as markupsafe cuts the escaped one.
- With no `promptEscaper` set, prompt mode escapes nothing (and writes no HTML entities).
- One environment escapes for one mode: HTML and prompt templates use separate environments.

The guarantee holds for the escaping and composing constructs. A template that edits escaped
text on purpose can still join two halves: slicing or reversing Markup, `trim` or `replace` of it
(the default separator is a space, which `trim` removes from the edges), or `neutralize` with
arguments of its own. As with `<|{{ role }}|>`, that is the template author's text.

`ConfigureForChatTemplates` does not switch prompt mode on: transformers renders with autoescape
off.

### `count_tokens`

The number of tokens in the value's `str()`, as an integer. The environment's `Tokenizer` counts
UTF-8 text (a wide template converts it first); an empty text is 0 without a call. Without a
tokenizer the count is the estimate `ceil(utf8_bytes / 4)`, which can undercount CJK text, digits
and code: a hard context limit needs a tokenizer. An exception from the callback fails the render
with `ErrorCode::UserCallableException`. Renders on several threads may call the callback at
once.

### `truncate_tokens`

`truncate_tokens(n, end="...")` returns the text unchanged when it is at most `n` tokens.
Otherwise it cuts it on a token boundary and appends `end`, so that the result, `end` included,
is at most `n` tokens; an `end` longer than `n` tokens is an error, as for Jinja2's `truncate`.
The cut uses the tokenizer's prefix function when the host gives one (a length past the end or
inside a UTF-8 sequence is clamped down), else a binary search on the count; without a tokenizer,
`4 * m` bytes rounded down to a code point. The result is a plain string.

### `pack`

`pack(budget, attribute=none, by=none, reverse=false, reserve=0, stop=false)` keeps the items of
a list whose text fits in `budget - reserve` tokens:

- The items are iterated once, so generators work, and kept as owned values.
- An item's text is `str(item[attribute])` (a dotted path, as `sort` and `map` take), else
  `str(item)`, measured before escaping.
- Items are considered in their order, or ordered as `sort(attribute=by, reverse=reverse)` would
  order them (`by='score', reverse=true` keeps the best retrieved chunks); without `by`,
  `reverse=true` considers the last item first (a history keeps the newest turns).
- An item is taken when it fits in what is left; one that does not is skipped, or with
  `stop=true` ends the packing (a history stays contiguous).
- The tokenizer runs at most once per item, and not at all after the budget is spent.
- The kept items come back in their original order, as a list. `reserve > budget` gives an
  empty list; a negative `budget` or `reserve` is an argument error.

```
{% for doc in docs|pack(3000, attribute='text', by='score', reverse=true) %}
{{ doc.text|fence }}
{% endfor %}
```
