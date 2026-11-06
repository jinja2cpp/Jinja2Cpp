"""String filters over a matrix of inputs (text, Unicode, Markup, numbers, none, URLs) and
arguments, autoescape off and on: the differential that 0107 found 830 differences with, kept so
each filter meets every kind of input (docs/tasks/0107)."""
INPUTS = ["'Hello wide World'", "'  a <b> & c  '", "'\\u00e9\\u4e2d x-y_z www.a.com'", "('<i>x</i> http://a.org'|safe)", "12", "2.5", "none", "''", "'one two\\tthree\\nfour five six seven eight'", "'see http://x.org/a?b=c&d and mail@x.org'"]
CALLS = ["replace('a', 'X')", "replace(' ', '')", "replace('<', '>')", "safe", "urlize", "urlize(10)", "urlize(nofollow=true, target='_b')",
         "center(5)", "center(30)", "trim", "trim(' a')", "truncate(5)", "truncate(9, true)", "truncate(5, end='')", "title", "wordcount",
         "wordwrap(6)", "wordwrap(6, false)", "indent(2, true)", "indent('> ', blank=true)", "urlencode", "capitalize", "striptags", "escape", "forceescape", "string", "length"]
CASES = []
n = 0
for i in INPUTS:
    for c in CALLS:
        for ae in (False, True):
            n += 1
            CASES.append((f"m{n:03d}", "[{{ " + i + "|" + c + " }}]", {"env": {"autoescape": ae}}))

# Case and digit properties that Unicode 16.0 (Python 3.14) added or changed: Garay letters and
# digits, U+019B/U+0264 gaining capitals, Cyrillic TJE; 15.1's CJK Extension I in wordcount. The
# pinned 3.12 (15.0) leaves each unmapped (docs/tasks/0372)
CASES += [
    ("unicode_16_0_upper_lower", r"{{ '\U00010d70ɤƛ'|upper == '\U00010d50ꟋꟜ' }}|{{ '\U00010d50ꟋᲉ'|lower == '\U00010d70ɤᲊ' }}"),
    ("unicode_16_0_title_capitalize", r"{{ '\U00010d70\U00010d71 ƛx'|title == '\U00010d50\U00010d71 Ƛx' }}|{{ 'ɤꟋ'|capitalize == 'Ɤɤ' }}"),
    ("unicode_16_0_int_float", r"{{ '\U00010d41\U00010d42'|int }}|{{ '\U00011bf7'|int(5) }}|{{ '\U000116d3.5'|float }}"),
    ("unicode_16_0_wordcount", r"{{ '\U00010d50 \U00010d41 \U0002ebf0'|wordcount }}"),
]
