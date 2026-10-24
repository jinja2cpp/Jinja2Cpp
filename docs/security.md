# Trust model

This page says which combinations of templates, data, loaders and host callables Jinja2C++
supports, what the library enforces for each, and what isolation the host has to provide
itself. It describes the 2.0 line. To report a vulnerability, see [SECURITY.md](../SECURITY.md).

Python Jinja2 needs a `SandboxedEnvironment` because a template can reach Python objects
through attributes. A Jinja2C++ template reaches only the values, callables and loaders the
host hands it, so there is no sandbox mode (see the Sandbox row in [parity.md](parity.md)).
A template can still ask for CPU time and memory, call host code and load files. The rest of
this page is about those.

## Tiers

| | Templates | Data | Configuration | Supported |
|---|---|---|---|---|
| Tier 0 | written by the host's developers | untrusted (user input, documents, API responses) | defaults | yes |
| Tier 1 | written by the host's users | untrusted | `UntrustedSettings()` (or `UntrustedLimits()` and tighter values), a loader whose root holds only templates (`ConfinedFileSystem` recommended), no callables the host would not hand to the template author | yes, with the limits listed below |
| | written by the host's users | any | default settings, or a loader root that holds other files | no: nothing bounds the render's time or memory, and every file under the root is readable |
| | hostile, several tenants in one process | any | any | no: run each tenant's renders in its own process or container (see [Containment](#containment)) |

**Tier 0.** Data never executes and never names a file: only template text reaches the
parser, and only names written in the template (or computed from data by template code the
host wrote) reach the loader. A crash, hang, unbounded allocation or memory error that data
alone can cause is a security bug. Autoescape protects this data as described in
[Escaping](#escaping).

**Tier 1.** In addition:

- Template text that crashes the process, corrupts memory or reads memory it should not is a
  security bug, whatever the template does.
- A template reaches files only through the configured loaders, by the name rules of
  [Loaders](#loaders). With `ConfinedFileSystem` it never reads a file outside the root, links
  included; with `RealFileSystem` the links inside the root are the host's choice.
- Every `Settings::Limits` field that is set ends a render that passes it with an error
  `Result`, code `ResourceLimitExceeded` (18) with the field name in `ExtraParams[1]`; depth
  limits report `RecursionLimitExceeded` (12). Load and Render report errors as a `Result`;
  only `std::bad_alloc` passes through, which the contract reserves for real system memory
  exhaustion (until [0263](tasks/0263-per-operation-size-checks.md) lands a template can still
  cause one, see [Known gaps](#known-gaps)).
- CPU time is bounded by `maxRenderSteps` and `maxRenderTime`, checked between engine steps.
- The template author sees whatever the template renders, and the text of any error the host
  shows them (see [Errors](#errors)).

What tier 1 does not yet deliver is listed under [Known gaps](#known-gaps): until those tasks
land, the memory one render uses is not bounded by the library alone.

## Escaping

Autoescape is off by default, as in Python; `UntrustedSettings()` turns it on, and
`Settings::autoescapeSelector` chooses it per template name like Python's `select_autoescape`.
With autoescape on, string data printed by `{{ }}` is escaped for `& < > " '`, which makes it
safe in HTML text and in quoted attribute values. It is not safe inside `<script>` or `<style>`,
in unquoted attributes, or as a URL (`javascript:`); `|tojson` escapes `< > & '` for embedding
JSON in HTML.

Autoescape protects against untrusted *data*, not untrusted *templates*: a template author can
always print raw HTML with `|safe`, `{% autoescape false %}` or by writing it as template text.
A `jinja2::Markup` value the host passes (or a callable returns) is trusted as safe HTML and is
printed unescaped; never build one from untrusted text.

## Host code

The host decides what a template can call: values and callables in the render parameters and
globals, filters and tests added with `AddFilter`/`AddTest`, reflected objects and their
accessors, `Settings::finalize`, gettext callables, custom `IFilesystemHandler`s. Jinja2C++
treats all of them as trusted host code:

- A callable runs with whatever arguments the template passes. Give a tier 1 template only
  callables that are safe for any arguments its author can write. A callable declared with a
  `*context` parameter sees the whole render context.
- Nothing preempts a running callable, accessor or loader. A callable that blocks or loops
  holds the render; `maxRenderTime` is checked when it returns.
- An exception a callable throws becomes an error `Result` (`UserCallableException`, 15)
  carrying its `what()` text.
- A render started from inside a callable gets the limits of its own template, not what is
  left of the outer render's budget, and shares the thread's stack check with it.
- Unknown filters and tests fail at Load; user callables are not usable as filters unless
  `Settings::userCallablesAsFilters` is set.

## Loaders

`MemoryFileSystem` serves only the names the host added.

`RealFileSystem` follows the name rules of Python's `FileSystemLoader` since 2.0:

- A name is split at `/`. A name with a `..` piece is not found.
- Empty and `.` pieces are skipped, so `/a.j2`, `./a.j2` and `a.j2` name the same file under
  the root; an absolute host path names a file under the root too, never the host file.
- On Windows a name holding `\` or `:` is not found: `dir\a.j2` is not another spelling of
  `dir/a.j2`, and a drive letter never names a host path.
- A name holding NUL is not found. Only regular files are found: no directories, FIFOs or
  devices.
- Symbolic links are followed wherever they point, as in Python. A root that holds links the
  host does not control is not confined.
- Failures give an empty stream or optional; `GetLastModificationDate` does not throw and no
  message carries the host path.

**1.x.** Up to 1.3.2, `RealFileSystem` appended the name to the root without checks: a name
with `../` pieces, or `../` pieces followed by an absolute path, opened files outside the root.
A 1.x host that loads templates whose names users can influence must vet those names itself.

`ConfinedFileSystem` (new in 2.0) follows the same name rules and opens a file only if the
file actually read lies under the root:

- The root path itself is trusted, links in it included. Below it, a link is followed only
  while it resolves inside the root: on POSIX an absolute link target, or a `..` that climbs
  above the root, is not found. On Windows the opened file's final path must lie under the
  root's final path.
- The check applies to the opened descriptor or handle, so a link swapped while a template
  loads cannot reach outside, and a FIFO or device is never read.
- Hard links and POSIX mount points under the root count as inside it.

Recommended for tier 1. Custom `IFilesystemHandler`s are host code: they enforce whatever
rules they implement.

The rules are pinned by the `RealFsNamesTest` and `ConfinedFsTest` rows in
[test/filesystem_handler_test.cpp](../test/filesystem_handler_test.cpp), which build a
disposable root with a file outside it, links leading in and out, a FIFO and a thread that
swaps a link while another loads through it.

## Resource limits

Built in, whatever the settings:

| Bound | Value | Reported as |
|---|---|---|
| Items or characters one checked operation builds (`range`, `*`, `center`, `indent`, `tojson`, ...) | 2^26 (`maxSequenceSize` can lower it) | 18 |
| Width and precision in `%` formatting | clamped to ±2^20 | no error |
| Width in string methods (`'x'.center(n)`, `ljust`, `zfill`, ...) | 2^28 (Python's `OverflowError`) | `TemplateRuntimeError` (17) |
| Thread stack | checked in the parser, the evaluator and deep-value walks | 12 |
| Template that includes, imports or extends itself | refused | 12 |

Nesting is bounded by `Settings::Limits` fields with defaults set close to where Python Jinja2
stops: render nesting `maxRenderDepth` 256, expression nesting `maxExpressionDepth` 70,
operator chains `maxExpressionOperators` 491, block nesting `maxBlockNesting` 248, each
reported as 12. These are defaults, not hard ceilings: a host can raise them, and 0 turns a
counter off, leaving only the thread stack check (with `maxBlockNesting` 0, deeply nested
blocks parse in quadratic time, [0282](tasks/0282-settings-limits-followups.md)). Keep the
defaults, or lower values, for tier 1.

`Settings::Limits` adds bounds that are off by default (see `template_env.h` for each field's
exact unit): `maxRange`, `maxSequenceSize`, `maxOutputSize`, `maxTemplateSize`,
`maxTemplateLoads`, `maxRenderSteps`, `maxRenderTime`. `UntrustedLimits()` sets values anchored
on Python's `SandboxedEnvironment`: `range()` of at most 100000 items, sequences of at most
2^20 items, 16 MiB of output, 10^7 steps, no time limit. The values may change between
releases. A step is an approximate unit that can change between releases too; set
`maxRenderSteps` with a wide margin over what the host's real templates take.

## Errors

Error text is meant for the host's developers. `ErrorInfo` carries the `what()` of host
exceptions and quotes the template line where the error happened, including lines of
templates the loader read. A host that shows errors to tier 1 template authors shows them that
text; filter it first if it may hold something the author should not see.

## Known gaps

Tier 1 claims stop here until these tasks land:

- Not every operation that grows a string or list checks `maxSequenceSize` before
  allocating (`~` and `+` concatenation, list `+`/`extend`, `join`, `replace`, `%` totals,
  `lipsum`, `wordwrap`); a template can build large values and run the process out of
  memory: [0263](tasks/0263-per-operation-size-checks.md).
- The template cache keys templates by their raw name and does not honour `cacheSize`, so a
  long-running host loading user-chosen names grows without bound; `MemoryFileSystem` fills
  its other-width copy in a const method without a lock:
  [0264](tasks/0264-template-cache-bound.md).
- One step, such as a filter over a long string, can overrun `maxRenderTime`:
  [0282](tasks/0282-settings-limits-followups.md).
- Hardening build flags and fuzz targets for untrusted templates:
  [0265](tasks/0265-hardening-by-default.md), [0267](tasks/0267-untrusted-mode-fuzzing.md).
- Cancellation, stack bounds for fibers and coroutines, error redaction:
  [0268](tasks/0268-untrusted-mode-followups.md).

## Never promised

- Preempting a running host callable, accessor, reflector or loader.
- A total memory bound per render without a host allocator or process limits. A host that
  needs one replaces `operator new` (or the allocator of [0252](design/0252-memory-contract-plan.md))
  and throws `jinja2::Exception(ErrorCode::ResourceLimitExceeded, message)` from it.
- Process isolation between renders or tenants.
- Timing and other side channels.
- XSS safety of a template's own output.
- Stack checks on stacks other than the OS thread's (fibers, coroutines).
- Builds with `-fno-exceptions`.

## Containment

For tier 1, and for any host where a runaway render must not take the service down, run
renders where the operating system bounds them: a worker process with a memory limit
(`RLIMIT_AS`, a cgroup, a Windows job object) and a wall-clock timeout the host enforces by
killing the worker. Use separate processes or containers for tenants that must not affect each
other. The library's limits make a well-behaved render fail early and cleanly; process limits
are the backstop for the gaps above and for host code.
