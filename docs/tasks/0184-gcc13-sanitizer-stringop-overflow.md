---
status: open
priority: low
area: build
depends: []
touches: [src/out_stream.cpp]
---
# The GCC 13 sanitizer build fails with -Wstringop-overflow

**Problem.** Found while building 0165: configuring master with GCC 13,
`-DJINJA2CPP_WITH_SANITIZERS=address+undefined -DCMAKE_BUILD_TYPE=RelWithDebInfo` fails to
compile with `-Wstringop-overflow` at src/out_stream.cpp:138 under `-Werror`. CI's
sanitizer job uses GCC 14 and passes, so only local builds with GCC 13 (the cloud
container's default) are affected.

**Proposal.** Check whether the warning is a GCC 13 false positive (known under ASan
instrumentation). If it is, silence it locally at that line with a comment naming the GCC
version. If it isn't, fix the copy.

**Done when.** The GCC 13 sanitizer configuration builds on master.
