---
status: open
priority: medium
area: standards
depends: [7]
---
# Decide the minimum supported C++ standard

**Problem.** The public API must stay C++14-compatible, which keeps the nonstd shims and
a C++14 googletest pin (v1.16.0; v1.17 needs C++17). Every new standard adds a CI row on
top. Keeping the floor costs maintenance; raising it may cost users on old toolchains.

**Proposal.** Find out who still builds as C++14 (issues, package-manager download stats,
downstream projects). If nobody, raise the floor to C++17 in the next major version and
drop the shims where `std::` equivalents exist.

**Done when.** The decision is recorded here and in the README.
