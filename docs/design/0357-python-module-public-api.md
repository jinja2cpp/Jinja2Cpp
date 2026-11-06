# 0357 plan: the Python module on the public API only

Status: PROPOSED, 2026-11-06. Awaiting Ruslan's decisions D1-D7 (section 5; D6 in detail in section 8; Codex review and resulting changes in section 9).
Checked against master 93392d4, PR #794 (branch `claude/project-thread-9gbcmg`, merge base master) and the described 0354 PR2/PR3. Written by the architect role; the advisor's second opinion (section 7) revises B1, B3, D3, D4 and D7 and the sequencing. Where they differ, section 7 is the current recommendation.

## 0. The tension

- **The obligation rule (Ruslan 2026-10-30, 0357 task file).** Before G1, the module must use only `include/jinja2cpp/`. Every capability it has must be one a C++ user can have too.
- **Drop-in parity and the perf gate (0353, 0355).** Python objects have to stay live and keep their identity (`x is y` through callables, across calls). They keep getattr-first lookup, `str()`/`repr()`/`__html__`, tuples, non-string keys and Undefined classes with name and hint. The module must also still beat Python Jinja2 on every `bench/cases` case. None of this exists in the public API today. The public `Value` has no object kind and no undefined (0072; 0212 D3/D4). Since 0212 P2 (#783), identity holds only inside one host call.

The private seam (0248 D3) solved both by sharing the internal `InternalValue` type. To keep both requirements, the replacement must be a public interface that is **narrow** (one small header) and **off the per-node path**. It must also not be **a second value type**.

## 1. Seam inventory

Mapping key: **(a)** existing public API covers it, **(b)** a small public addition is needed (named in section 2), **(c)** the code moves into the module.

### 1.1 Headers (master, then #794)

- `python/src/common.h:10` includes `internal_value.h`. Every module file gets the engine's types through it.
- `adapters.h:6` (`host_object.h`).
- `adapters.cpp:5-8` (`expression_evaluator.h`, `generic_adapters.h`, `host_object.h`, `internal_value.h`).
- `convert.cpp:6-15` (`big_int.h`, `cycle_collector.h`, `expression_evaluator.h`, `host_object.h`, `internal_value.h`, `out_stream.h`, `recursion_guard.h`, `render_context.h`, `undefined.h`, `value_visitors.h`).
- `environment.cpp:7-10` (`host_object.h`, `internal_value.h`, `recursion_guard.h`, `template_impl.h`).
- `errors.cpp:3` (`error_boundary.h`).
- #794 adds: `context.cpp:6` (`render_context.h`), `poll.h:5` (`recursion_guard.h`), `template_module.cpp:9-11` (`host_object.h`, `internal_value.h`, `template_impl.h`).
- Build: `python/CMakeLists.txt:45-48` puts `src/` on the include path and links `JINJA2CPP_PRIVATE_LIBS` (fmt). `CMakeLists.txt:84-86` requires a static build: "the module uses internal symbols a shared library hides".
- `loaders.cpp` is already public-only (`IFilesystemHandler`, `ConfinedFileSystem`, `TemplateEnv::AddFilesystemHandler`, `loaders.cpp:92-388`), apart from the transitive `common.h`.

### 1.2 By capability

| Capability | Seam use (file:line) | Mapping |
|---|---|---|
| **Value in: scalars, str, Markup** | `InternalValue::FromText` + `SetMarkup` (`convert.cpp:39-47`); `NoneValue` (605); bool and double ctors (608-631) | (a) `Value(std::string)`, `Value(Markup)`, `NoneValue`. **Cost:** a str is copied twice (Python → `std::string` → `InternalValue::FromText`, `value_visitors.h:542-556`), once today |
| **Value in: int beyond int64** | `IntegerFromDigits(hex)` (`convert.cpp:72`) | (b) B5: `BigInt::FromDigits(digits, base)`. The public `BigInt` is decimal only (`big_int.h:74`), and decimal conversion in Python hits `sys.get_int_max_str_digits` |
| **Value in: dict/list/tuple/other objects** | `MakeHostValue<PyHost>` (`convert.cpp:649,659,670`); `PyHost : IHostObject` (`adapters.h:20-80`); `PySequenceItems : IndexedListAccessorImpl` (`adapters.cpp:66-95`); `ListAdapter::CreateAdapter` over a generator (`adapters.cpp:155-160`) | (b) B1: a public host-object interface. The iterators are (a) with public `IIndexBasedAccessor`/`MakeGenericList`. **Risk:** the live-length loop rule (`adapters.cpp:60-64`) must still hold through `GenericListAdapter` |
| **Value out: scalars, str, Markup, big int** | `ValueKind` switch, `GetIf`, `AsStringView`, `ConvertString`, `AsBigInt`, `MagnitudeInBase` (`convert.cpp:318-366`) | (a) `jinja2::Visit` over `Value::GetData()`, `Value::AsMarkup`. Big int is (b) B5 `BigInt::ToDigits(16)` |
| **Value out: template lists/dicts, range, pairs** | `AsList`/`AsMap`, `GetRangeInfo`, `GetIdentity` memo, `IsTuple`, `KeyValuePair` (`convert.cpp:251-302,370-374`) | (a) `GenericList`/`GenericMap`/`ValuesList`/`ValuesMap` as the export views of 0331/0348 give them. (c) The cycle memo is keyed by accessor address in the module. **Divergence to accept:** a `range` reaches Python as a `list` |
| **Host object identity** (`x is y`, dict keys, `sameas`) | `IHostObject::Identity` (`adapters.h:61`); `static_cast<const PyHost*>(GetHostObject())` (`adapters.cpp:443-447`, `convert.cpp:281-284`) | (b) B1. The public host object *is* the `GenericMap` accessor, so a callable gets back the host's own `shared_ptr`. Identity then holds **across calls**, not only inside one call as 0212 P2 gives it (`0212-plan.md` section 10) |
| **Host attrs, items, slices, calls, iteration, len, truth, `==`, hash, mapping/sequence/tuple flags** | the 20 overrides at `adapters.h:42-66`, `adapters.cpp:213-440`; `host::MakeCall` (`convert.cpp:516`) | (b) B1, the same operations over `Value` |
| **Markup / `__html__` / escaping** | `IsMarkupLike`/`Html` (`adapters.h:65-66`, `adapters.cpp:429-441`); `markupType` checks (`convert.cpp:345, 633-641`) | (a) public `Markup` and `Escape`. `__html__` is (b) B1 (`IsMarkupLike`/`Html` stay on the interface) |
| **Undefined in** (a Python `Undefined` instance in params or a return value) | `UndefinedInfo`, `UndefinedValue`, `MakeRef` (`convert.cpp:183-207`) | (b) B2. The public `Value` has no undefined (0072) |
| **Undefined out** (to a Python callable, with name and hint) | `GetUndefinedInfo`, `UndefinedMessage` (`convert.cpp:209-238`) | (b) B2 (opt-in per callable). Today a public callable gets `None` (0212 D4) |
| **Undefined policy** | `UndefinedPolicy` (`environment.cpp:69-99`, `convert.cpp:74-103`) | (a) `template_env.h:38` |
| **Python filters, tests, finalize, globals as callables** | `BorrowedCallable` = `Callable(Callable::UserCallable, lambda(CallParams, RenderContext&))` (`convert.cpp:673-677`); `host::EngineUserCallable` (`environment.cpp:289,542`; #794 `environment.cpp:508,550`, `template_module.cpp:129,133`) | (a) public `UserCallable{fn, {VarArgs, VarKwArgs}}` (#645 passthrough). Host arguments keep their identity through B1. **Cost:** `PrepareUserCallableParams` builds `ValuesMap` + `ValuesList` + a `ContextMapper` `make_shared` per call (`internal_value.cpp:4600-4625`), where today it is a direct `CallParams` → vectorcall (`adapters.cpp:449-481`) |
| **Template values lent to Python** (macro, `caller`, loop, cycler, namespace) | `TemplateValue`, `CallLease`, `LeaseScope` (`convert.h:58-127`); `CycleCollector::Current/Reentry`, `HomeCall::Restore`, `RenderToString`/`OutStream` (`convert.cpp:456,522-534`); `Subscript` (555) | (a) A lent `UserCallable` already does reentry, restore and `TemplateExpired` (`internal_value.cpp:4313-4354`, api-2.0 3.10 items 8-9). Attributes are read with `GenericMap::Find`. (c) The lease bookkeeping moves into the module. **Change:** a kept `loop` stays readable as the 0331 snapshot after the call instead of raising, which is closer to Python |
| **str/repr/type name of template values** | `visitors::ValueRenderer`, `PythonTypeNameGetter` (`convert.cpp:579-597`) | (a) `Value::ToString` for str. (c) repr and type name are made in the module. Type names such as `LoopContext` become generic: an accepted divergence, or B1-style `GetObjectClass` later |
| **Errors / exception mapping** | `ErrorInfo`, `ErrorCode`, `GetException` + rethrow (`errors.cpp:81-115`) is public. `detail::CurrentExceptionToError` (`errors.cpp:122`) is private | (a) for the first. (c) for the second: catch `jinja2::ErrorInfo`/`jinja2::Exception`/`std::exception` around lent-callable calls. **Verify in S1:** a lent macro call that fails must throw public types only. If an internal type escapes, that is a public-API bug to fix in the engine, not new API |
| **Template loading, env, settings** | `TemplateEnv`, `Settings`, `AutoescapeSelector`, `finalize` (`environment.cpp:284-289,450,507-560`) | (a) |
| **Render entry + output** | `TemplateImpl<char>::RenderInternal(tpl, out, InternalValueMap, &globals)` (`environment.cpp:735`; #794 `:762`; `src/template_impl.h:390-406`) | (b) B3: a public `RenderTo` overload with `RenderOptions{globals}`. The params become a `ValuesMap` of `Value`s (host objects as `GenericMap`), built once per render |
| **Ctrl-C polling** | `detail::t_hostPoll` (`environment.cpp:37-52`; #794 `poll.h:22-37`; `src/recursion_guard.h:142-146`) | (b) B3: `RenderOptions::poll`. C++ users get cooperative cancellation |
| **Recursion and cycle guards** | `StackNearlyExhausted` (`convert.cpp:312`; #794 `template_module.cpp:54,254`) | (c) `Py_EnterRecursiveCall` (limited API) plus the module's own depth counter. Cycles: (c) the address memo, as above |
| **Context proxy, pass_context** (#794) | `RenderContext::FindValue`, `CollectVisibleNames`, `GetTemplateFrame()->name`, `IsAutoescape` (`context.cpp:82-90,110-147,162-180`) | (a) `ArgInfo::Context` (`value.h:1015`) gives `GenericMap(ContextMapper)`, and `Find` covers lookup. (b) B4: `ContextMapper::GetKeys` returns the visible names; it returns `{}` today (`internal_value.cpp:4144`). This is behaviour only, no header change. (b) B4: template name and autoescape at the call site |
| **make_module** (#794) | public `Template::MakeModule` (`template_module.cpp:357`) plus `EngineUserCallable` and `Value2IntValue` (`template_module.cpp:129-133,214,299`) | (a) for `MakeModule`. (c) Value → Python conversion in the module. Python args go in as B1 host values instead of eager `ValuesMap` copies (`template_module.cpp:61-133`) |
| **meta.find_referenced_templates** (#794) | `TemplateImpl<char>::ReferencedTemplatesOf` (#794 `src/template_impl.h:+391-395`, `template_module.cpp:395`) | (b) B6: `Template::FindReferencedTemplates()`, next to `FindUndeclaredVariables` (`template.h:390`) |
| **Sandbox** (0354 PR2, described only) | `IHostSandbox` hook, `UndefinedInfo.security`, `Dialect::ImmutableSandbox`, the CallExpression path | Split. (c) The policy for Python objects goes in `PyHost::GetAttr`/`Call`, the one checkpoint of 0248 D8. (b) B7: the part for engine-native values (immutable containers, unsafe calls on template values, `SecurityError`). Its shape should be fixed with the 0354 thread before PR2 is pushed |
| **Fallback** (0354 PR3) | Python only | none |

## 2. Proposed public additions

Format as in `docs/api-2.0.md` section 4. All are C++17 and additive. There is no ABI promise before G1, and the inline namespace `v2` stays.

**B1. `jinja2::IHostObject`, new header `include/jinja2cpp/host_object.h`**
- `struct JINJA2CPP_EXPORT IHostObject : IMapItemAccessor` with the operations of today's private interface over `Value`: `TypeName`, `Str`, `Repr`; `GetAttr(std::string_view)` and `GetItem(const Value&)` returning `optional<Value>`; `GetSlice`; `IsCallable`/`Call(const UserCallableParams&)`; `IsIterable`/`Iterate() -> optional<GenericList>`, `Reversed`; `Contains`, `Len`, `Truth`, `Equals`, `Hash`, `Identity`; `IsMapping`/`IsSequence`/`IsTuple`, `IsMarkupLike`/`Html`.
- `IMapItemAccessor`'s own members get final defaults on top: `GetSize` = `Len`, `Find` = attr-then-item, `GetKeys` = a mapping's keys. A C++ reader of the `GenericMap` sees the object as a Jinja2 template would.
- One new virtual on `IMapItemAccessor`: `virtual const IHostObject* AsHostObject() const noexcept { return nullptr; }`, asked only where a `GenericMap` enters the engine (`InputValueConvertor`, `value_visitors.h:618-628`). The engine marks the value as host there (`MapAdapter::MarkAsHost`, `internal_value.h:1033`). The per-node check stays the `m_isHost` bit, so **the native per-node path is unchanged**.
- `src/host_object.h` becomes an engine-only bridge: one internal implementation that delegates to the public object and converts `Value` ↔ `InternalValue`. Its ~90 engine call sites stay.
- Identity: `ArgumentToHost` hands a host value back as the host's own `shared_ptr` (today a `HostObjectView`, `host_object.cpp:29,178-181`).
- C++ users gain opaque objects with getattr, printing and calls; stable identity across calls, as dict keys and with `sameas`; tuples that print as tuples; non-string keys.
- ABI: one vtable slot on `IMapItemAccessor`, no new `ValueData` alternative, `sizeof(Value)` unchanged.
- **A genuine new obligation.** About 25 virtuals frozen at G1. Recommendation: a `Query(HostQuery, ...)` extension point (D3).

**B2. Undefined across the host interface**
- `struct UndefinedDescription { UndefinedPolicy policy; std::string name, hint; }` and `virtual const UndefinedDescription* AsUndefined() const { return nullptr; }` on B1.
- Inbound: a host object that answers `AsUndefined()` becomes the engine's undefined. Outbound: a `UserCallable` with the new field `bool keepUndefined = false` set gets an undefined argument as a `GenericMap` whose `AsHostObject()->AsUndefined()` is set. Every other callable still gets `None` (0212 D4 unchanged).
- C++ users gain: a callable can tell undefined from none.

**B3. `struct RenderOptions` with `Template::RenderTo(StringType&, const ValuesMap&, const RenderOptions&)` and its sink form**
- `const ValuesMap* globals = nullptr`: replaces the environment's globals for this render and its imports (as `RenderInternal`, `template_impl.h:416-481`).
- `void (*poll)(void*) = nullptr; void* pollArg = nullptr;` called every `HostPollSteps` (1024) steps, may throw. Replaces `t_hostPoll`.
- C++ users gain per-render globals, cancellation and timeouts. Perf: same `RenderCore<true>` instantiation; the poll is the existing thread-local check.

**B4. Call-site information**
- `ContextMapper::GetKeys()` returns the visible names (behaviour only, row in `test/user_callable_test.cpp`).
- New field `UserCallableParams::site` (`std::string_view templateName; bool autoescape;`), filled only when `argsInfo` asks for `*context`.
- C++ users gain Jinja2's `pass_eval_context` information.

**B5. `BigInt::FromDigits(std::string_view, int base, bool negative)` and `BigInt::ToDigits(int base) const`**, exported, out of line, any backend.

**B6. `Template::FindReferencedTemplates() -> Result<std::vector<std::optional<std::string>>>`**, the public form of #794's `ReferencedTemplatesOf`.

**B7 (with 0354 PR2). Sandbox for engine-native values**
- `Settings::immutableContainers` (public name for the internal dialect bit the chat preset already sets); `ErrorCode::SecurityError`.
- Only if PR2 really needs a callback on engine callables: `Settings::sandboxPolicy` (`shared_ptr<const ISandboxPolicy>`, equality by identity). Otherwise the Python policy stays inside `PyHost` (c).
- C++ users gain a sandbox switch for untrusted templates.

**Perf check.** None of B1-B7 adds work per node for native values. Host path only: a str copied twice; one more allocation per container read (`make_shared<PyHost>` plus the engine adapter); per Python-callable call `ValuesList`/`ValuesMap` plus `ContextMapper` allocations instead of a direct `CallParams`. The bench decides (S1b); fallback in D2.

## 3. Options

1. **Public API as it is** (GenericMap/GenericList/UserCallable, `JsonTraits` snapshots). Loses 0248 D3 drop-in semantics (opaque objects, `str(obj)`, tuples, non-string keys, identity across calls); 0355 pass rate drops a lot. Rejected: fails "the upstream pass rate matches".
2. **A Value-based public host-object interface: B1-B7 (recommended).** One public header (~150 lines) and an engine bridge (~400 lines); rewrite of `convert.cpp`/`adapters.cpp` (~1500 lines touched); 25 virtuals frozen at G1; extra copies/allocations on the host path only.
3. **A "binding SDK" tier** `include/jinja2cpp/ext/` exposing engine-value types with no ABI promise (static link only). Cheapest, no perf change, but it is the second contract the task exists to remove, under another name. Fallback only if speed of delivery beats a smaller obligation (D1).

**Recommendation: option 2.** If the bench shows a losing case, add the result-sink variant (D2) before G1, because changing B1's signatures afterwards breaks the frozen ABI.

## 4. Steps (PR-sized), sequenced against 0354

The 0354 thread owns `python/src/` and `src/template_impl.h` until its PR1 (#794), PR2 and PR3 have merged.

- **S0. Decision record and baseline (now).** B1-B7 into `docs/api-2.0.md` section 4 as "proposed (0357)", 0357 line in `release-2.0.md` G1. Baseline on master + #794 in the task file: `python/bench/bench_module.py` (25 cases, Load/Render ratio), module pytest count, corpus-through-module count, 0355 upstream rate (or its harness once by hand). Send the 0354 thread the B7 shape.
- **S1a. Public `IHostObject` + engine bridge (B1, B2).** `include/jinja2cpp/host_object.h` (new), `value.h` (`AsHostObject`), `src/value_visitors.h#InputValueConvertor`, `src/host_object.{h,cpp}`, `src/internal_value.cpp#ArgumentToHost`. Not `python/src` or `template_impl.h`. Tests: new `test/host_object_test.cpp` with a toy C++ host object: getattr before item; `x[0:2:1]`; tuple repr `(1, 2)`; `{{ obj }}` via `Str`; `__html__` under autoescape; `{ident(o): 1}[o]` and `ident(o) is sameas o` across two calls; `is mapping`; `dictsort`; undefined in/out with and without `keepUndefined`. ASan. `bench/run.py` native row unchanged.
- **S1b. Perf probe (throwaway branch).** Port only `PyHost` to B1 on S1a + #794, run `bench_module.py`. Freeze the signatures if every case still beats Jinja2 and none loses more than 5% of its S0 margin; otherwise open D2.
- **S2a. B4 and B5 (now).** `internal_value.cpp#ContextMapper`/`PrepareUserCallableParams`, `value.h` (`site`), `big_int.h`, `src/big_int.cpp`. Rows in `test/user_callable_test.cpp`, `test/big_int_test.cpp` (both backends).
- **S2b. B3 and B6, after #794 merges.** Public `RenderTo(..., RenderOptions)` over `RenderCore<true>`; poll via the same thread-local; `FindReferencedTemplates` wraps #794's code. Tests in `test/template_env_test.cpp` (globals override, import sees it, throwing poll) and `test/referenced_templates_test.cpp` through the public method (builds in shared mode too, removing #794's CMake exclusion).
- **S2c. B7, after PR2 merges.** Public setting for PR2's hook; PR2 sandbox rows via `Settings`.
- **S3. Switch `python/src`, after 0354 PR3 merges.** Two green commits: (1) convert/adapters on B1, public `UserCallable`, leases via lent callables; (2) environment/errors/context/template_module/poll on B3/B4/B6 and public error mapping. `common.h` stops including `internal_value.h`. Tests: `python/tests` under ASan + refcount loop; corpus count = S0; 0355 rate = S0 (or each changed id explained); bench beats Jinja2 on all 25 and within S1b tolerance.
- **S4. Installed-package build and engine cleanup (just before G1).** `python/CMakeLists.txt` standalone via `find_package(jinja2cpp)`, no `src/` or `JINJA2CPP_PRIVATE_LIBS`; static-only check (`CMakeLists.txt:84-86`) dropped for external mode, static + `--exclude-libs` kept for wheels. New `python.yml` job: install the library (static, PIC, `JINJA2CPP_WITH_BIGINT=boost`) to `$PREFIX`, build `python/` alone against it, guard that includes resolve only to `$PREFIX/include`, nanobind and system dirs, run pytest. Delete `RenderInternal`, `ReferencedTemplatesOf`, `host::EngineUserCallable`, external `MakeHostValue` use and external `t_hostPoll` users. `src/host_object.h` stays engine-only (the bridge). `test/package` lists `host_object.h`. Docs: `python.md`, api-2.0 status, task done.

**Order:** S0, S1a, S2a now. S1b after S1a. S2b after #794. S2c after PR2. S3 after PR3 and S1b. S4 last.

## 5. Decisions for Ruslan

- **D1. Approach.** (a) option 2: public `IHostObject` plus small additions; (b) option 3: an unstable `ext/` tier; (c) option 1: public API as is, Python semantics lost. **Recommend (a).**
- **D2. B1 result shape.** (a) `optional<Value>` returns (an extra copy per host str and container read); (b) an opaque exported `HostValueSink`/`HostArg` (one copy, larger surface). **Recommend (a), conditional on S1b**; switch to (b) before G1 only if a case loses or drops more than 5% of its margin.
- **D3. Growth of a frozen interface.** (a) `virtual std::optional<Value> Query(HostQuery, const Value&) const` with an enum extended in 2.x; (b) no extension point. **Recommend (a).**
- **D4. Undefined to C++ callables (B2).** (a) opt-in `keepUndefined`; (b) no public undefined; (c) a `ValueData` alternative (reopens 0072). **Recommend (a).**
- **D5. Kept template objects.** A `loop` or namespace kept by Python after the call becomes a readable snapshot instead of raising (as 0331 for C++). **Recommend accept.** Lent macros still expire (0300).
- **D6. Sandbox for engine values (B7).** (a) settings switch + `ErrorCode::SecurityError`, no host callback; (b) public `ISandboxPolicy` callback. **Recommend (a)** unless PR2 shows `test_security.py` needs per-call host decisions on engine values.
- **D7. Gate tolerance.** "Beats Jinja2 on every case" stays the hard gate; each case may also lose at most 5% of its S0 margin. **Recommend yes.**

## 6. What this makes hard next

- **B1 is frozen at G1.** Later Python protocols go through `Query` (D3a) or wait for 3.0.
- **The host path pays for the public boundary.** If chat-template workloads grow, D2's sink variant becomes a 2.x addition (`IHostObject2`).
- **C++ users get a third container kind** with Python-like semantics; docs and parity table must say where a host object differs from a `GenericMap`.
- **Merge-train risk.** S3 is a large rewrite that must wait for PR3; new 0354 work after PR3 should target B1 directly.

## 7. Advisor revisions (2026-11-06, current recommendation)

The advisor agrees with option 2 (D1a), D2a (conditional on S1b), D5 and D6a, and changes the shape:

- **B1 by composition, not inheritance.** `IHostObject` stands alone (~21 virtuals with defaults) and does not derive from `IMapItemAccessor`. Inheriting would give C++ users two vocabularies on one object: `Value::IsMap()` true for a list-like host object while `is mapping` is false, `GenericMap::begin()` iterating empty `GetKeys()` while `GetSize()` returns `Len()`, `Find` meaning attr-then-item. Instead the library exports `Value MakeHostValue(std::shared_ptr<const IHostObject>)` (plus header-only `MakeHostValue<T>(args...)` allocating object and bridge together). The bridge is the library's own accessor deriving from both public `IMapItemAccessor` and internal `IMapAccessor`, so allocations equal the plan's. One slot on `IMapItemAccessor`: `AsHostObject()`, overridden by the bridge only; `GenericMap::AsHostObject()` returns `std::shared_ptr<const IHostObject>` via the aliasing constructor (no raw pointer in user code). `m_isHost` stays the per-node check.
- **B2/D4: free functions instead of an interface slot.** `Value MakeUndefined(UndefinedDescription)` and `std::optional<UndefinedDescription> UndefinedOf(const Value&)`; the carrier stays internal; opt-in `keepUndefined` on `UserCallable` stays. Trap: `UserCallable`'s hand-written copy/move/assign (value.h:1118-1148) must copy the new field.
- **B3 poll as `std::function<void()>`**, matching every other public callback. A poll alone must activate the limits path: `HasRenderLimits` (recursion_guard.h:203-206) ignores the poll today. `const ValuesMap* globals` is a documented call-scoped borrow (or `optional<reference_wrapper>`).
- **D3: no `Query(HostQuery, Value)` escape hatch.** Grow by `IHostObject2 : IHostObject`, recognised once by `dynamic_cast` when the bridge is made and cached. Growth rule written in api-2.0.md.
- **D7: measure the 5% margin in instructions** (or min-of-N), not single wall-time runs; "beats Jinja2 on every case" stays the hard gate.
- **Inventory gap (a→b):** callable members of lent template objects (`loop.cycle`, `cycler.next`, `ns.fn`, `caller` inside a dict) convert to None on the public path (`HostCallableOf`, internal_value.cpp:4060,4747); only top-level arguments are lent (`ArgumentToHost`, :4390). Engine fix (no header change): the exported view of a lent object lends its callable members under the same lease. Macro attributes (`m.name`, `m.arguments`) are lost: accept as divergence or add later. `str(namespace)`/`str(loop)` print a dict, not `<Namespace {...}>`: list for 0355.
- **Sequencing:** S1a rewrites `src/host_object.h` into the bridge, which collides with 0354 PR2's `IHostSandbox` hook and `UndefinedInfo.security` in `src/`. Settle D6 first: PR2 keeps the Python policy inside `PyHost::GetAttr/Call`, or S1a lands before PR2. PR3 (fallback) is python/ only, so S3 can start after PR2 instead of after PR3.
- **Next PR:** S0 + these decisions recorded in api-2.0.md section 4, then S1a with the composition design, the lent-member fix and `test/host_object_test.cpp` (21 tests today against the private seam) over the public header.

## 8. D6 in detail: what 0354 PR2 (sandbox) would put in the engine

0354 PR2 is not pushed yet. It is on that thread's branch `sandbox-pr`, at 03f0127. Its `src/` part adds:
- `IHostSandbox`, a thread-local host callback installed for each render, with `NativeAttribute` and `Format` queries;
- `UndefinedInfo.security` and `SecurityErrorMarker`;
- `Dialect::ImmutableSandbox`;
- a method-call path in `CallExpression`.

There are two ways to go:
- **D6a (recommended): cut PR2 down to the B7 shape.** That shape has four parts: a `Settings` switch for immutable containers, `ErrorCode::SecurityError`, engine-side refusal of `_`-prefixed names, and a native sandboxed `format`. The policy for Python objects stays inside `PyHost::GetAttr/Call`.
  - Cost: a few parity cases on error-message text are lost.
  - Cost: overrides of `is_safe_attribute`/`is_safe_callable` in a user subclass of `SandboxedEnvironment` no longer reach engine-native values, only Python objects.
- **D6b: keep the host callback, and 0357 makes it a public `ISandboxPolicy`.**
  - It gives full parity for subclassed sandboxes.
  - Cost: one more interface frozen at G1.
  - Cost: a host call on the attribute and format paths of engine values, in sandboxed renders only.

## 9. Codex review (2026-11-06, third opinion)

Codex agrees with option 2 and with composition (D1). It also agrees with D3: an `IHostObject2` found and cached once, not `Query`. It agrees with D4: free undefined functions plus an opt-in `keepUndefined`, with `None` staying the default. It found four gaps in the plan and disagrees on D6. The cited code was checked, and every claim holds.

- **Native dict keys are lost.** A template dict exported through the public view stringifies non-string keys, and keys that spell alike collapse (`SharedDictAdapter::CreateSnapshot`, `src/internal_value.cpp:3261-3279`). The module today converts the original keys through `GetEntries` (`python/src/convert.cpp:297-302`). After the switch, `record({1: 'a', '1': 'b'})` would get one entry instead of two.
  - Needed: a public, lossless way to enumerate entries, for example `GenericMap` entries whose keys are `Value`s. The alternative is that Ruslan accepts this as a divergence.
- **Native tuples are lost.** `OutputValueConvertor` exports a `ListRef` as a plain `GenericList` (`src/internal_value.cpp:4057`), so the tuple flag on `ListAdapter` does not reach the public side. `python/tests/test_values.py:146-153` needs a nested template tuple to arrive in Python as a tuple.
  - Needed: a tuple flag on the public list export, for example `GenericList::IsTuple()`, plus a row in S1a.
- **The cycle memo can't key on accessor addresses.** `CreateGenericList()` allocates a new accessor on every call (`src/internal_value.cpp:2617-2626`). So `{% set xs = [] %}{% do xs.append(xs) %}{{ record(xs) }}` never sees the same address twice, while the module today keys its memo on the underlying list (`python/src/convert.cpp:258-266`).
  - Needed: a stable public identity token on exported `GenericList`/`GenericMap`, which can be the same `Identity()` notion as B1.
- **The perf claim is too broad.** `InputValueConvertor` also runs during evaluation, not only when a render starts. A native generic map's `HostItem` goes through `LendGenericItem` and `Value2IntValue` for a nested map (`src/internal_value.cpp:1280-1294, 3498-3511`). So the `AsHostObject()` probe adds one virtual call each time a nested native `GenericMap` is read. That read already allocates an adapter, so the relative cost is small, but it is not zero.
  - Needed: limit the "unchanged" claim to the `IsHost()` dispatch sites, and add instruction-count coverage in S1b for nested generic, reflected and JSON maps.
- **D6: Codex disagrees with the switch-only sandbox.** Under B7 alone, a subclassed `SandboxedEnvironment` that rejects a native method never sees that operation. Native methods are resolved and called directly (`src/expression_evaluator.cpp:1685-1696`), and native attribute safety is decided in `src/value_methods.cpp:2747-2764`. So the loss is in what the policy enforces, not only in message text. Codex prefers an optional public policy callback, with no cost when it is unset, honoured on both the native attribute path and the direct method-call path. A fixed built-in sandbox is acceptable only as an explicit, owner-approved reduction of the drop-in contract.

**Effect on the plan.** B1 gains three export capabilities: lossless entries, the tuple flag and the identity token. All three belong to the public `GenericList`/`GenericMap` export views and also help C++ users. The D6 recommendation changes to **D6b, an optional `ISandboxPolicy`**, because parity in what the policy enforces is a goal and ordinary renders pay nothing.
