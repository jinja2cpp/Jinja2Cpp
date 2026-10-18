# C++-only filters

Jinja2C++ adds a few filters that Python Jinja2 does not have. A template that uses them
does not load in Jinja2 (`No filter named ...`). Since 2.0 each has the output described
here (docs/tasks/0233); before, `camelize`, `underscorize`, `escapecpp`, `toxml` and
`toyaml` were registered but rendered an empty string.

| Filter | Output |
|---|---|
| `applymacro(macro=name, ...)` | The value passed to the macro `name`, with the other arguments |
| `camelize(upper_first=true)` | An identifier in CamelCase |
| `underscorize` | An identifier in snake_case |
| `escapecpp` | The text escaped for a C++ string literal |
| `toyaml` | The value as a block-style YAML document |

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

- `none` (and a plain undefined) is `null`, booleans `true`/`false`, integers in decimal,
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
- `namespace()`, `cycler()`, `joiner()`, macros and strict undefined values raise an error.

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
