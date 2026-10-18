---
status: open
priority: high
area: api
depends: [0172]
touches: [include/jinja2cpp/error_info.h, include/jinja2cpp/value.h, docs/api-2.0.md]
---
# One error contract for the 2.0 public API

**Release impact (0221).** This decides part of the public API being frozen for 2.0;
settle it before the 0076 header freeze. Needs an architect plan first.

**Problem.** The public API reports errors in several ways, with no written contract:
- `Load` and `Render` return `Result<T>` with `ErrorInfo`; engine exceptions are caught at
  the render boundary (`src/template_impl.h`, the `catch` chain in `Render`) and mapped
  to an `ErrorCode`, everything unknown to `UnexpectedException` with only `what()` and
  no source location.
- Public headers throw standard exceptions from accessors: `std::bad_variant_access` from
  `Value::As<T>()`/`AsX()`, `std::out_of_range` from `OrderedMap::at`.
- Since 0172, building a `Value` (or `Reflect`) from an unsigned integer above
  `INT64_MAX` throws `std::overflow_error`: the first throw while the host assembles its
  data rather than while it reads it back.
- Lazily reflected values (JSON bindings, reflected containers, user callables) throw in
  the middle of a render, and the host gets the vague `UnexpectedException`.
- There is no jinja2cpp exception type, and `noexcept` guarantees and the
  "exceptions required" build requirement (`-fno-exceptions` is unsupported) are not
  documented.

**Questions for the plan.**
1. Which operations may throw to the host, and which return `Result`? Is the throwing
   `Value` constructor kept, or paired with a non-throwing checked factory?
2. Specific `ErrorCode`s for host-data errors (out-of-range integer, failed reflection,
   exception from a user callable), carrying the template location where the failing
   value was used, instead of `UnexpectedException`.
3. Whether jinja2cpp needs its own exception type (one base deriving from
   `std::runtime_error`) or keeps standard types only.
4. How exceptions from user callables and custom reflectors are translated, and whether
   the original exception is kept (`std::exception_ptr` in `ErrorInfo`).
5. Where the contract is documented (README section and `docs/api-2.0.md`).

**Done when.** The plan is decided, the contract is written in `docs/api-2.0.md`, and
implementation tasks are filed for any API changes it needs.
