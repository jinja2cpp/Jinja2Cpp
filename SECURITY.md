# Security policy

## Reporting a vulnerability

Report vulnerabilities privately through GitHub: open the repository's **Security** tab and
choose **Report a vulnerability**. Please do not open a public issue, pull request or
discussion for a suspected vulnerability.

Include the Jinja2C++ version or commit, the compiler and platform, the settings and loader
used, and the smallest template and data that show the problem. Use disposable files in a
reproduction, never real secrets or host files.

We acknowledge a report within 7 days and keep the reporter informed while we work on it.

## What counts

[docs/security.md](docs/security.md) states the trust model: which combinations of templates,
data, loaders and host callables are supported, and what the library enforces for each. A
report is a vulnerability when a supported combination breaks one of those guarantees, for
example:

- untrusted data, or a template in the untrusted tier, crashes the process, corrupts memory
  or reads memory it should not;
- a template reads a file through `RealFileSystem` or `ConfinedFileSystem` that their name
  rules refuse, or through `ConfinedFileSystem` outside its root;
- with autoescape on, untrusted string data printed by `{{ }}` breaks out of HTML text or a
  quoted attribute;
- a template passes a `Settings::Limits` bound without the render failing.

Behaviour that page lists as never promised or as a known gap is not a vulnerability, though
a report that a known gap is worse than described is welcome.

## Supported versions

| Version | Security fixes |
|---|---|
| 2.x, latest minor release | yes |
| older 2.x minor releases | no: upgrade to the latest minor |
| 1.x | no |

## Disclosure

We aim to release a fix within 90 days of the report and coordinate the disclosure date with
the reporter. Fixes are announced in a GitHub security advisory for this repository, with a
CVE requested through GitHub when the issue affects a release. Reporters are credited in the
advisory unless they ask not to be.
