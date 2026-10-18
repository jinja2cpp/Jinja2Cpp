---
status: open
priority: high
area: docs
depends: [0239]
touches: ["site:docs/", "site:_includes/", "site:index.md"]
---
# Website: 1.x and 2.x documentation side by side

Paths prefixed `site:` are in jinja2cpp/jinja2cpp.github.io.

**Problem.** The site has one unversioned tree. Its download page says 1.2.1 (the last
release is 1.3.2, 2024-06-23), the changelog stops at 1.2.1, and the only API reference is
1.0. 2.0 renames much of the API, so 1.x users still need their docs while 2.x becomes
current. README links (`/docs/usage`, `/docs/build_and_install.html`,
`/docs/j2_compatibility.html`, `/docs/usage/ud_callables.html`) must keep working.

**Proposal.** Freeze the current content under `/1.x/` with a "this is 1.x, see 2.0"
banner; 2.x at the root; a version selector in the header include; jekyll-redirect-from
for moved URLs. 0227 records which 1.x pages are kept; this task is the mechanism.

**Done when.** Both versions build, every old URL resolves, the selector works, and the
changelog covers 1.3.x and 2.0.
