---
status: open
priority: high
area: docs
touches: ["site:_config.yml", "site:Gemfile", "site:.github/", "site:_layouts/"]
---
# Website: pinned build, CI deploy and link check

Paths prefixed `site:` are in jinja2cpp/jinja2cpp.github.io.

**Problem.** The site (https://jinja2cpp.github.io) is built by GitHub Pages' classic
branch build with `remote_theme: pmarsceill/just-the-docs`: the theme's old repo name,
not pinned, so the theme can change under the site at any time. There is no Gemfile and
no `.github/`, so nobody can build or preview the site the way Pages does, and nothing
checks links. `color_scheme: dark` is forced. `_layouts/default.html.b` is dead.
Most content dates from 2019-2020 (0221 scout, 2026-10-18).

**Proposal.** A Gemfile with Jekyll 4 and just-the-docs as a pinned gem; a
`.github/workflows/pages.yml` that builds and deploys with actions/deploy-pages; a PR job
that builds and runs a link checker (lychee or html-proofer; external links soft-fail at
first). Migrate config keys to the current theme, add its light/dark toggle, delete the
dead layout. This is the base for 0240-0243.

**Done when.** The site builds locally and in CI from pinned versions, deploys from the
workflow, and PRs get a build plus link report.
