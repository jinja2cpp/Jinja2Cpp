# 0252 plan: allocator and memory contract for the 2.0 public API

Architect plan, 2026-10-18, on master `48f6983` . Status: **approved by Ruslan
2026-10-18 14:31** ("go recommended": R1-R4 as recommended in section 10). Implementation tasks:
T1 = [0256](../tasks/0256-survive-allocation-failure.md), T2 = [0257](../tasks/0257-render-to-string-and-sink.md),
T3 = [0275](../tasks/0275-release-thread-caches.md), T4 = [0276](../tasks/0276-memory-contract-docs.md). The joint section with 0250 (error contract) is section 6. It is aligned with the
approved 0250 contract (docs PR #584, docs/api-2.0.md 2.1): `bad_alloc` propagates and means only
"the system is out of memory"; every limit and every spent memory budget is
`ErrorCode::ResourceLimitExceeded`, and there is no `OutOfMemory` code. Revised 2026-10-18 12:30:
budget rule aligned with #584, `ContainerSink` adapter added (Ruslan's fmt-buffer question). Claims marked *probe* were run against a fresh RelWithDebInfo build of `48f6983`
(GCC 13, libstdc++, x86-64); the rest come from reading the code.

## 0. Facts this plan rests on (two correct the brief)

1. **The parse-tree arena does not use pmr.** `grep` finds no `std::pmr`, `boost::container::pmr`,
   `memory_resource` or `bad_alloc` in `src/` or `include/`. As built (0118 P3/P4a), `NodeArena`
   bump-allocates from a 512 B inline block, then `new std::byte[]` blocks doubling to 64 KB
   (src/node_arena.h:1259-1283). `Seal` copies everything into one exact-size buffer owned by
   `SealedArena` (`unique_ptr<std::byte[]>`). There is no re-parse on overflow and no internal use
   of `bad_alloc`. The `monotonic_buffer_resource` + null-upstream design in
   docs/design/0118-parse-tree-arena-plan.md sections 4 and 10 was superseded. Rule kept for the
   future: an internal "arena full" signal must never be `std::bad_alloc`. It would have to be a
   private type, so that a host OOM stays distinguishable.
2. **Allocation failure crashes today.** *Probe*: global `operator new` replaced so that it fails
   exactly the n-th allocation, swept over a template using `for`, `set`, `join` and a macro:
   - `Template::Load`: 19 of 24 failure points throw `std::bad_alloc` out of `Load`; **4 crash**
     (SIGSEGV).
   - `RenderAsString`: 87 of 96 return `ErrorCode::UnexpectedException` ("std::bad_alloc");
     **8 crash**.
   - With memory exhausted (every later allocation fails too), `RenderAsString` throws
     `bad_alloc`: the catch handler's own error building allocates (src/template_impl.h:414-423).

   Causes:
   - (a) `NodeArena::Allocate` appends the `Block` *before* `new std::byte[blockSize]` succeeds
     (node_arena.h:1269-1270). The empty block (data null, start 0) then resolves every older
     node to `nullptr + offset` in `DestroyNodes` (backtrace: `ArenaNode::GetKind this=0x1e8`).
   - (b) Vendored robin_hood tables are destroyed after an allocation fails inside insert/rehash
     (robin_hood.h:2480-2487, `Table::destroy`). This is reached from `ScopeStack::pop_back`
     (render_context.h:298), from `RenderWorkspace::RenderMaps`, and from the static builtin-globals
     tables (global_functions.cpp:524/530).
   - Read, not probed: `std::length_error("a template too large for its arena")`
     (node_arena.h:1264) also escapes `Load` as an exception, since the parser catches only
     `runtime_error`.
3. **std::pmr on Apple is a run-time floor, not a compile-time one.**
   - LLVM 18 libc++ `__availability`: std::pmr needs macOS 14, iOS 17, tvOS 17 or watchOS 10. The
     availability markup is compiled out (`#if 0`, llvm issue 40340), so back-deploying compiles,
     then fails at load time. `memory_resource::~memory_resource()` and its typeinfo live in the
     dylib (`__memory_resource/memory_resource.h:29-33`).
   - macos-build.yml sets no `CMAKE_OSX_DEPLOYMENT_TARGET`, so the macos-14/15 runners target
     their host OS and would never see the problem. Linux CI has no libc++ row.
   - libstdc++ (GCC >= 9), MSVC (>= 15.6) and LLVM libc++ (>= 16) all ship it.
   - `boost::container::pmr` avoids the floor, but 0230 (Boost-free core) removes
     `boost_container`, so it cannot appear in a public header.
4. **Sizes** (*probe*, GCC 13): `Value` 40 B, `ValuesList` 24, `ValuesMap` 80; `std::string` 32
   against `std::pmr::string` 40; `vector<Value>` 24 against `pmr::vector<Value>` 32.
5. **Host data is already borrowed per render.** Render converts only the top-level params, into
   the workspace-lent `InternalValueMap`, with `InputValueConvertor(false, true)`
   (template_impl.h:333): strings become views and containers go through adapters (0115). The copy
   a host pays is when it *builds* a `ValuesMap`. `Value` already has `string_view`/`wstring_view`,
   `GenericList` and `GenericMap` alternatives, and `Reflect(const T&)` borrows.
6. **Thread-local state** (all grow-only until thread exit):
   - `RenderWorkspace`: slot chunks of 256 slots or more, doubling; at most 16 loop frames; at most
     4 pairs of lent maps of up to 1024 buckets; the last globals snapshot.
   - `LookupCache`: 128 entries, fixed.
   - HTML-escape buffer, up to about 20 KB (markup.h:64).
   - Pending-destroy array, `realloc`'d (internal_value.cpp:51).
   - Recursion `ThreadStack`.

   The globals snapshot holds the `shared_ptr<const ValuesMap>` of the last environment that
   rendered on the thread. **The env's globals, and the captures of its global callables, outlive
   the `TemplateEnv` on every such thread** until another env renders there or the thread ends.
7. **Output paths.**
   - `Render(ostream&)` renders into a temporary string and writes it only on success
     (template.cpp:105-116): all-or-nothing, with the whole output held at the end.
   - `RenderAsString` makes one new string per render, reserved from the size hint.
   - The internal `TemplateImpl::Render(basic_string&)` already *appends* (it reserves from
     `os.size()`).
   - Python 3.1.6: `generate()` yields `['a','1','b']` before a `ZeroDivisionError`; a `MemoryError`
     propagates to the caller.
8. **Allocation counts.**
   - Render is not allocation-bound: allocator swaps leave the Render geomean at 1.00-1.06
     (perf-track/2026-10-06-allocators.md, which concluded "no allocator dependency").
   - Many renders allocate in single digits (for_filter_if 6, many_tags 2: wave2-verdicts #457, #461).
   - `Load/many_tags` makes about 958 allocations, of which the sealed buffer is one and the arena
     blocks a handful. The rest are strings, values and tables owned by nodes, on global `new`.

## 1. The tension

Embedders who budget memory want to say where Jinja2C++ allocates, and to hear what it keeps.
Every hook that says where becomes API and ABI that 2.x must keep, and its lifetime rule ("the
resource outlives …") must hold for objects whose last owner is hard to predict: Template copies,
the env cache, macro callables escaping through `weak_ptr`, and other threads. Pushing the hook
into the value model costs size on the paths 0140/0151 shrank. And today the more basic promise,
"an allocation failure is survivable", does not hold (fact 2).

## 2. Options

| | What 2.0 ships | Cost |
|---|---|---|
| **A (recommended)** | No allocator parameters. A written memory contract; OOM made survivable (`bad_alloc` propagates cleanly, as 0250 R2 has it); caller-owned output (`RenderTo` into a string or a sink); `ReleaseThreadCaches()` | 4 render overloads, 1 interface, 1 free function. No layout change to `Value`/`Settings`. Hooks stay addable in 2.x as new `TemplateEnv` methods or overloads |
| B | A plus `std::pmr::memory_resource&` hooks: an arena upstream on `TemplateEnv`/`Settings`, and a per-render scratch resource in a `RenderOptions` struct | Covers 1 of about 958 Load allocations (fact 8) unless every node member moves to the arena; the lifetime rule reaches into `weak_ptr` escapes; render values escape a per-render resource (UAF class); Apple floor for users of it (fact 3); render gains nothing measurable |
| C | B plus an allocator-aware value model (`pmr` strings/containers in `Value`, `ValuesList`, `ValuesMap`) | `Value` 40 -> 48 B; copies silently drop to the default resource (pmr does not propagate on copy); every signature taking them changes; no hot-path gain, since rendering uses `InternalValue` (0140) and borrows host data anyway (fact 5) |

## 3. Decisions on the six questions

**Q1. Entry points taking a memory resource: none in 2.0, and no `RenderOptions` struct yet.**
- Load upstream: the template's retained memory is the sealed buffer plus what nodes own on global
  `new`. A hook covering the first would mislead a host that thinks "my arena holds the template".
- Its lifetime rule ("outlives every copy of the Template, the env cache, and every escaped macro
  callable") cannot be checked by anyone.
- It becomes honest once nodes own nothing (0118's later phases); then it can be added *additively*
  as a `TemplateEnv` method, since `TemplateEnv` is pimpl'd and nothing changes layout.
- Per-render scratch: the approved perf design (perf-design-overview.md row 10) rejected per-render
  arenas for the reused thread workspace. Render is not allocation-bound (fact 8). Values escape a
  render (user callables keep `Value`s; `loop`/macro callables; the globals snapshot), so a
  per-render resource is a use-after-free class.
- With no field to carry, an options struct now would only be ABI to keep.
- Guidance for when one is needed (2.x): pass it as a new overload by `const&`; make its first
  member `std::uint32_t structSize = sizeof(RenderOptions)`, and have the library read only the
  prefix it knows. Fields can then be appended without the SOVERSION bump `Settings` costs.
- Type, if a hook ever comes: `std::pmr::memory_resource&` (a reference: non-null, non-owning,
  borrowed for the documented scope), never `boost::container::pmr` (0230). The library must not
  call `get_default_resource`/`new_delete_resource`, so that only users of the hook carry the
  Apple floor.

**Q2. `Value`, `ValuesList`, `ValuesMap` stay on `std::allocator`.** Option C's costs (table) buy
nothing on the hot path. Hosts that keep data in their own arena present it without copies:
- `Value(std::string_view)` for arena strings;
- `Reflect(const T&)` (borrows) for structs;
- an `IMapItemAccessor`/`IListItemAccessor` over arena containers (`Find(string_view)`, 0115).

Contract: borrowed data stays valid until the render call returns, and for as long as the host
keeps any `Value` that a user callable received during it.

**Q3. Output: add `RenderTo(StringType&)` (appends) and `RenderTo(BasicOutputSink&)` (streams).**
- **Append rather than clear.** It composes (a header and a body into one buffer), and reuse is
  `out.clear()` on the caller's side. Clear semantics cannot express append.
- **On error the string is truncated back to its size at entry**: no allocation; capacity may have
  grown.
- **Sink.** `Write(view)` is called in order on the rendering thread, with the 512-character
  `OutStream` buffer or a long fragment straight from the template source (0113). It is never
  called with an empty view, and the view is valid during the call only. Chunks written before an
  error stay written, as with Python's `generate()`.
- **An exception thrown by `Write`** ends the render and is reported by 0250's rule for host-code
  exceptions.
- **`Render(ostream&)` keeps its all-or-nothing behaviour** (now documented). Streaming to a stream
  is a five-line sink. This avoids silently changing what a failed render leaves in a file (R3).
- **Overloads rather than a target-handle type**: each new target is an ABI-additive overload,
  where a by-value handle would change layout when a target kind is added. No raw pointer appears.
  The `GenericMap` forms follow the existing template trick that keeps `{}` unambiguous.
- **Measured by** new bench families `RenderTo/<case>` (reused string) and `RenderSink/<case>`
  (counting sink) next to `Render/<case>` (T2).

**Q4. The thread-local workspace: keep it, and add `ReleaseThreadCaches()`; no caller-passed
workspace.**
- It is the approved design (row 10): stack discipline already handles a user callable rendering
  a nested template on the same thread.
- `ReleaseThreadCaches()` frees, on the calling thread only: the workspace's slot chunks, loop
  frames, lent maps and globals snapshot (fixing fact 6's env retention for hosts that care), the
  escape buffer and the empty pending-destroy array.
- It returns `false` and frees nothing while a render runs on the thread: called from a user
  callable or a sink.
- It is `noexcept` (only frees), and touches nothing shared, so it needs no lock.
- It is defined out of line in the library, so it releases the instance the library uses: the
  shared-library caveat in render_workspace.h:38-39 holds even more under 0076's hidden
  visibility.
- Uses: thread pools trimming idle workers; plugins before `dlclose`, since glibc keeps a DSO with
  registered TLS destructors loaded; tests checking the env-lifetime fact.
- A caller-passed workspace is rejected for 2.0 (section 9). It can still come as an additive
  overload later, at the cost of routing every `ForThisThread()` site through the context.

**Q5. Out of memory: `std::bad_alloc` propagates unchanged from every call, once the library is
exception-safe under it. No `OutOfMemory` code (aligned with 0250's draft R2/D5).**
- **Today it is neither** (fact 2): `Load` throws, `Render` disguises the failure as
  `UnexpectedException` with a handler that itself allocates, and both can crash. The crash fixes
  and an OOM sweep test (T1) are a G8 robustness bug whatever the reporting channel.
- **Why propagate rather than a `Result` code:**
  - Python raises `MemoryError` to the caller.
  - No error object has to be built after an allocation failed.
  - It is the standard library's convention.
  - Template-driven oversized requests already fail earlier, as `Result` errors, before they
    allocate (`MaxSequenceSize`, 0185; 0097's limits). What remains is real memory pressure, which
    the host's handler should see as such.

  Cost: a host built with `-fno-exceptions` cannot catch it; such hosts usually run a
  non-throwing `new` anyway.
- **Guarantees, all checked by T1's sweep:**
  - a failed `Load` keeps the previously loaded template (strong, already so for parse errors,
    template_impl.h:258-299);
  - a failed `LoadTemplate` leaves the env cache unchanged;
  - a failed render leaves the `Template` and `TemplateEnv` unchanged, and the thread caches
    usable (RAII `WorkspaceMaps`): the next render on the thread succeeds;
  - `RenderTo(string&)` truncates on *any* exit, `bad_alloc` included (a scope guard; shrinking
    never allocates);
  - a sink keeps what it received.
- **Budgets are not `bad_alloc`** (0250, #584 2.1 item 2). A memory budget that runs out, whether
  one of the library's limits (`Settings::Limits`, 0262; the built-in caps) or one the host
  enforces in its own code (a callable, a sink, a replaced `operator new`, a future memory-resource
  hook), is reported as `ErrorCode::ResourceLimitExceeded`. A host reports a spent budget by
  throwing `jinja2::Exception{ResourceLimitExceeded}`, which does not derive from `bad_alloc` and so
  becomes a `Result` error instead of propagating. `bad_alloc` is kept for real system OOM.
- **The arena's `std::length_error`** (node_arena.h:1050, 1070, 1115, 1264) is a `Load` failure:
  `ResourceLimitExceeded` with `extraParams[1]` naming the arena size limit (#584 2.1 item 2).
- **Internal rule:** nothing uses `bad_alloc` as an internal signal (fact 1).
- **`noexcept`:** `Load*`/`Render*` cannot be `noexcept`, since they propagate `bad_alloc` and run
  host code. 0250's R6 table owns the list; 0252 adds `ReleaseThreadCaches() noexcept`.

**Q6. Documentation.**
- docs/api-2.0.md:
  - section 2 gets two convention lines: "no allocator parameters in 2.0; memory contract in 3.9"
    and the OOM rule;
  - new rows in 3.2 (`RenderTo`, `BasicOutputSink`) and 3.3 (what the env caches);
  - a new **3.9 Memory and allocation** inventory (fact 6 and section 5 below);
  - decision 7, "Memory contract (0252)", with these answers;
  - section 10 rows for the rejected options.
- The user-facing text (section 5) goes in README as a "Memory and threads" subsection directly
  under the "Compatibility and versioning" section that 0076 PR-1 adds.
- Doxygen `\par Memory` notes on `Load`, `RenderTo`, `RenderAsString`, `TemplateEnv`, and
  `ReleaseThreadCaches`.
- MIGRATION.md (0077) gets two notes: `Render` no longer reports `bad_alloc` as
  `UnexpectedException` (it propagates, per 0250), and `RenderAsString` callers that reuse buffers
  can switch to `RenderTo`.
- docs/security.md (0224) links to the contract for untrusted templates (limits are 0097's).

## 4. New public API (exact)

```c++
// include/jinja2cpp/template.h, inside namespace jinja2 (inline namespace v2 after 0076)

//! Receives a render's output in order (RenderTo). Implemented by the host; not owned by the library.
template<typename CharT>
class BasicOutputSink
{
public:
    //! `text` is never empty and is valid during the call only. Called on the rendering thread.
    //! An exception thrown here ends the render (reported as 0250 decides). No virtual function
    //! will be added in 2.x.
    virtual void Write(std::basic_string_view<CharT> text) = 0;

protected:
    BasicOutputSink() = default;
    BasicOutputSink(const BasicOutputSink&) = default;
    BasicOutputSink& operator=(const BasicOutputSink&) = default;
    ~BasicOutputSink() = default; // never deleted through the base
};
using OutputSink = BasicOutputSink<char>;
using OutputSinkW = BasicOutputSink<wchar_t>;

//! Appends a render's output to any container with `append(first, last)`: std::basic_string,
//! fmt::basic_memory_buffer, std::vector, boost::container::small_vector. Header-only and compiled
//! in the caller, so the library takes no dependency on the container's library. `out` must
//! outlive the sink; the sink must outlive the RenderTo call.
template<typename Container, typename CharT = typename Container::value_type>
class ContainerSink final : public BasicOutputSink<CharT>
{
public:
    explicit ContainerSink(Container& out) noexcept : m_out(out) {}
    void Write(std::basic_string_view<CharT> text) override { m_out.append(text.data(), text.data() + text.size()); }

private:
    Container& m_out;
};

// in template<typename CharT> class BasicTemplate, public:
//! Appends the output to `out`; on error `out` is truncated back to its size at entry
[[nodiscard]] Result<void, CharT> RenderTo(StringType& out, const ValuesMap& params) const;
template<typename Map, std::enable_if_t<std::is_same_v<Map, GenericMap>, int> = 0>
[[nodiscard]] Result<void, CharT> RenderTo(StringType& out, const Map& params) const
{
    return RenderToGeneric(out, params);
}
//! Streams the output to `sink`; what was written before an error stays written
[[nodiscard]] Result<void, CharT> RenderTo(BasicOutputSink<CharT>& sink, const ValuesMap& params) const;
template<typename Map, std::enable_if_t<std::is_same_v<Map, GenericMap>, int> = 0>
[[nodiscard]] Result<void, CharT> RenderTo(BasicOutputSink<CharT>& sink, const Map& params) const
{
    return RenderToGeneric(sink, params);
}
// private, exported with the class:
Result<void, CharT> RenderToGeneric(StringType& out, const GenericMap& params) const;
Result<void, CharT> RenderToGeneric(BasicOutputSink<CharT>& sink, const GenericMap& params) const;

//! Frees what renders on the calling thread keep for the next render (slot frames, loop frames,
//! parameter tables, the last environment's globals, buffers). Returns false, and frees nothing,
//! when a render is running on this thread. The next render on the thread allocates them again.
JINJA2CPP_EXPORT bool ReleaseThreadCaches() noexcept;

```

- `[[nodiscard]]` on `RenderTo` follows whatever 0080/D6 decide for `Render`; D6 recommends it.
- `ContainerSink` keeps a reference, not a pointer: it is a short-lived adapter on the caller's
  stack (`fmt::memory_buffer buf; ContainerSink sink(buf); tpl.RenderTo(sink, params);`). Unlike
  `RenderTo(string&)` it does not truncate on error; the container keeps what was written, as any
  sink does.
- `RenderTo(std::string{}, …)` does not compile: a temporary does not bind to `StringType&`.
- fwd.h (0076 PR-1) gains `template<typename CharT> class BasicOutputSink;` and the two aliases;
  test/fwd_test.cpp uses them.
- ABI: the new functions are additions (abidiff bit 4, reported only). `BasicOutputSink` has
  inline members only, so it needs no `JINJA2CPP_EXPORT` (no MSVC C4251/C4910 concern), and its
  one-slot vtable is frozen for 2.x.

## 5. The memory contract, user-facing draft (README "Memory and threads")

- **Allocation.** Jinja2C++ allocates with global `operator new`/`malloc` only, and takes no
  allocator or memory resource in 2.0. To route or cap all of it, replace the global `operator new`
  (standard C++). It has no allocator dependency.
- **`Template::Load`** copies the source, builds the parse tree, and frees the parser's transient
  memory before it returns. The loaded template keeps its source, its tree (one buffer) and what
  the nodes own (constants, names, tables). A template typically keeps 1.4 to 30 times its source
  (bench/README "Memory"). A failed `Load` keeps the template loaded before.
- **`TemplateEnv`** keeps up to `Settings::cacheSize` (default 400) parsed templates, its globals,
  filters and tests. Destroying it drops the cache.
- **Rendering** allocates for the values the template creates (lists, strings, filter results)
  and frees them before returning.
  - `RenderAsString` allocates its result.
  - `RenderTo(string&)` allocates only when `out` lacks capacity.
  - `RenderTo(sink)` never holds the output.
  - `Render(ostream&)` holds the whole output until the render succeeds.
  - Parameters are read in place: strings, lists and maps you pass are not copied.
- **Per thread.** Each thread that renders keeps reusable buffers for its next render: frames,
  parameter tables (bounded), an escape buffer, and the globals of the last environment it rendered
  with, which keeps them alive after that `TemplateEnv` is destroyed. `ReleaseThreadCaches()`
  frees them on the calling thread, for example before an idle pool thread parks or before
  unloading a plugin.
- **Process-wide.** The builtin globals and filter tables are built on first use and live until
  exit.
- **Out of memory.** A failed allocation throws `std::bad_alloc` out of the call that hit it, as
  Python raises `MemoryError`. `bad_alloc` means only that the system is out of memory: if your
  code enforces a memory budget (in a callable, a sink or your allocator), signal a spent budget by
  throwing `jinja2::Exception` with `ErrorCode::ResourceLimitExceeded`, and the call returns it as a
  `Result` error like the library's own limits. The template, the environment and your output string are left as
  they were: a failed `Load` keeps the template loaded before. A template that asks for an absurd
  size (`'x' * 10**12`, a huge `range|list`) fails earlier, as a `Result` error.
- **Untrusted templates.** Use `Settings::Limits` (0262; 0097, 0098) to bound what one template can
  ask for.

## 6. Joint section with 0250

0250 (approved; docs PR #584) is aligned on R1 (host exceptions become `Result` errors carrying
an `exception_ptr`), R2 (`bad_alloc` propagates only for system OOM; limits and spent budgets are
`ResourceLimitExceeded`, a host signals one with `jinja2::Exception{ResourceLimitExceeded}`; no
`OutOfMemory`), R3 (output on failure) and R6 (the `noexcept` table). What 0252 needs from it:

1. **`OutputSink::Write` is host code.** Its exceptions are classified like a host callable's:
   0250's code 15 with the pointer; a `jinja2::Exception` keeps its code (so a sink enforcing an
   output budget throws `jinja2::Exception{ResourceLimitExceeded}`); `bad_alloc` propagates.
   `Write` runs inside statement bodies (a flush every 512 characters) *and* after the root body
   (the final `Flush`), so the classification is needed at the render boundary as well as at
   `ComposedRenderer`'s catch.
2. **R3 can be specific in 2.0.** #584 2.1 item 3 leaves a caller-supplied stream's content after a
   failure unspecified; the targets 0252 adds (and `Render(ostream&)` under R3 here) can say more:
   - `RenderTo(string&)`: the string is truncated to its size at entry;
   - `RenderTo(sink)`: the sink keeps what it received;
   - `Render(ostream&)`: writes only on success;
   - `RenderAsString`: no string.

   0250's doc table should state these.
3. **R2 is false until T1 lands.** "`bad_alloc` propagates unchanged" does not hold while `Load`
   and `Render` can crash (fact 2). 0250's boundary PR (`catch (const std::bad_alloc&) { throw; }`)
   lands with or after T1 and runs its sweep. That PR flips one expectation in test/oom_test.cpp
   (Render: `UnexpectedException` -> `bad_alloc`).
4. **The arena's `length_error`** at `Load` (fact 2) is settled by #584: `ResourceLimitExceeded`,
   naming the arena limit in `extraParams[1]`. T1 adds the oom_test row; 0250's boundary PR maps it.
5. **R6 gains `ReleaseThreadCaches() noexcept`.**

## 7. Work breakdown (T1 = 0256, T2 = 0257, T3 = 0275, T4 = 0276)

The 0252 PR itself (docs only):
- docs/api-2.0.md decisions (section 3, Q6);
- the task files T1-T4;
- a note to the merge steward to add T2 and T3 to the G1 API-wave list in docs/release-2.0.md
  (0221's file).

Order: T1 now (a bug, G8), before 0250's boundary PR. T2 after 0076 PR-1 (fwd.h exists) and after
0250's classification code exists (for sink exceptions). T2 and T3 land before the RC freeze, in
either order relative to 0076 PR-2 (its script re-runs). T4 last.

| # | Work | touches | Tests | Bench / gate |
|---|---|---|---|---|
| **T1** | Survive allocation failure. `NodeArena::Allocate`: allocate into a local `unique_ptr`, then `emplace_back` (and audit `m_objects.Reserve`, `GrowSymbols`). robin_hood: fix `Table` so that a failed `initData`/`rehashPowerOfTwo` leaves a destroyable table; if that takes more than about 40 lines in the vendored file, add "passes the OOM sweep" to 0231's done criteria and land those rows with the successor. Audit the catch handlers that allocate | src/node_arena.h, src/robin_hood.h (or via 0231), test/oom_test.cpp (new), CMakeLists.txt (a second test executable `jinja2cpp_oom_tests`, since it replaces global `operator new`) | A parameterised table of templates (the probe's, plus include/extends/import, filters, `{% set %}` blocks, i18n, wide): for each, Load and Render fail allocation n = 0..N (until one succeeds). Expect: no crash or leak (ASan/LSan via ctest in the sanitizer jobs); outcome `ok`, a `Result` error, or `bad_alloc` thrown out of the call (Render: `UnexpectedException` until 0250's boundary PR flips that one expectation); after any failure the same `Template` renders correctly on the same thread; a failed Load leaves the previous template in place. Also "exhausted" mode (every later allocation fails) | Cold paths only. `Load/*` and `Render/*` within ±0.1% instructions on `count.py --baseline` (the benchmark.yml `instructions` job, 3% gate) |
| **T2** | `RenderTo(string&)` and `RenderTo(BasicOutputSink&)`, narrow and wide. `OutStream` gets a sink target: `FlushBuffer`/`WriteLong` call `sink.Write`, and the `WriteBuffer`/`WriteValue` fast paths are unchanged. Truncate on any exit (scope guard). Sink exceptions classified per section 6.1. fwd.h entries. Header-only `ContainerSink` adapter (section 4). Bench families `RenderTo/<case>` and `RenderSink/<case>` for plain_text, substitute, large_static, mitsuhiko_table and mitsuhiko_table_wide | include/jinja2cpp/template.h, include/jinja2cpp/fwd.h, src/template.cpp, src/template_impl.h, src/out_stream.h, src/out_stream.cpp, test/output_test.cpp, test/template_api_test.cpp, test/fwd_test.cpp, bench/jinja2cpp_bench.cpp, bench/README.md | output_test: `ExpectRender` also renders with `RenderTo` into a pre-filled string and into a recording sink, so **every existing OutputTest row** checks that the three paths agree, narrow and wide. New rows: output past the 512 buffer, long fragments arriving as their own chunk, an error mid-render (string truncated to the prefix; sink kept the chunks), a sink that throws (0250's host code and pointer), a user callable rendering another template to a sink during a render, `ContainerSink` over `std::string`, `std::vector<char>` and `fmt::memory_buffer` (test only; fmt stays private), a sink throwing `jinja2::Exception{ResourceLimitExceeded}` (returned as that code). template_api_test: `static_assert`s on the signatures; `RenderTo(std::string{}, …)` does not compile (a detection-idiom check) | Existing `Render/*` within ±0.3%, watching `plain_text` and `large_static` (they rose in 0113). Expected: `RenderTo/*` makes one allocation fewer per render than `Render/*`, with instructions down (plain_text -8..-15% on its ~986, large cases ±0.5%). `RenderSink/mitsuhiko_table` `Peak` far below `Render`'s 347 KB. `RenderSink/*` through `ContainerSink<fmt::memory_buffer>` within ±0.5% of `RenderTo/*`. Confirm with `run.py` |
| **T3** | `ReleaseThreadCaches()`. `RenderWorkspace::Release()`, which refuses while `FrameCount() != 0` or maps are lent; `LoopFrames` cleared; `Globals()` reset; HTML-escape buffer and pending-destroy array shrunk (when empty) | include/jinja2cpp/template.h, src/template.cpp, src/render_workspace.h, src/render_workspace.cpp, src/markup.h, src/internal_value.cpp, test/template_api_test.cpp | Render, release (`true`), render again with identical output. From a user callable during a render: `false`. Env retention: a global callable capturing a `shared_ptr` sentinel; destroy the env; the sentinel is still held; `ReleaseThreadCaches()`; the sentinel is freed. TSan job: four threads rendering and releasing in turn | Not on the render path: all cases ±0.1% |
| **T4** | User docs: README "Memory and threads" (section 5 text, adjusted to what landed); docs/api-2.0.md 3.9 final; Doxygen notes; the texts sent to 0077 (MIGRATION) and 0224 (security.md) | README.md, docs/api-2.0.md | G6 snippet compile covers the README examples (`RenderTo`, sink, `ReleaseThreadCaches`) | — |

Follow-up notes for other tasks (no new tasks):
- 0231: the successor must be exception-safe under `bad_alloc` and pass `jinja2cpp_oom_tests`.
- 0262 (`Settings::Limits`): consider an output-size limit, checked at `OutStream` flush (a cold
  path, once per 512 characters), failing with `ResourceLimitExceeded`. It bounds a render's
  output memory without an allocator.
- 0095: a fuzz target variant that fails the n-th allocation (expected outcome: `bad_alloc` or a
  `Result`, never a crash).

## 8. What this makes hard next

- **Exception safety under `bad_alloc` becomes a promise.** Every new entry point, container and
  catch handler must keep it. The OOM sweep test is a permanent CI cost (seconds) and a constraint
  on 0231 and any future container. Switching later to an `OutOfMemory` `Result` code would be an
  observable contract change.
- **The sink's single virtual is frozen** for 2.x. A richer protocol (flush, cancel) means a second
  interface and overloads.
- **Truncate-on-error and append** fix `RenderTo(string&)` semantics. A later "keep partial output"
  wish needs a new target, not a flag.
- **`ReleaseThreadCaches()` commits the thread-cache design to being releasable.** Any future
  per-thread cache (a 0231 table pool, an interner) must honour it, and must refuse while a render
  runs.
- **"No allocator parameters" is a 2.0 statement, not a forever one.** A later hook goes on the
  pimpl'd `TemplateEnv` or a new overload, so it stays ABI-additive. Writing the contract in terms
  of "global operator new" means a hook has to say exactly which allocations move.

## 9. Considered and rejected

- **`std::pmr::memory_resource*` in `Settings`** (perf-track allocator note's option).
  - It covers one allocation of a Load (fact 8).
  - Its lifetime spans every copy, the cache and `weak_ptr` escapes.
  - It adds a pointer to a user-allocated, compared struct (`operator==`).
  - It brings the Apple floor (fact 3).
  - Revisit as a `TemplateEnv` method once nodes own nothing.
- **`boost::container::pmr` in the public API**: contradicts 0230.
- **An own `jinja2::MemoryResource` interface**: it has the same lifetime problems, and reinvents a
  standard type. It is only worth having if a hook is needed *and* the Apple floor matters.
- **A per-render scratch resource or `RenderOptions` now**: render is not allocation-bound, values
  escape renders, the workspace already reuses (design row 10), and there is no field to carry.
- **pmr `Value`/`ValuesList`/`ValuesMap`**: +8 B per `Value`, the non-propagating copy trap, every
  signature changes, and no hot-path gain (fact 5).
- **A caller-passed `RenderWorkspace`**:
  - it exposes internal frames and maps as public types;
  - nested renders from user callables would not receive it;
  - every `ForThisThread()` site (statements.cpp loop-frame pool, globals, maps) would route
    through the context;
  - the TLS access it saves is a few instructions.
- **A `std::function` or template callback instead of the sink**: `std::function` may allocate on
  construction, and a template cannot be compiled into the library. A host wraps a lambda in five
  lines.
- **`RenderTo(OutputIt)`**: it would be a header wrapper over the sink anyway; `back_inserter`
  users get `RenderTo(string&)`.
- **A fixed `span<CharT>` target (`ToChars`-style)**: it has no clean meaning for a partial render,
  and the sink covers it.
- **`bool Write()` to cancel**: a render that prints nothing never reaches it. Cancellation belongs
  to a general budget or deadline (0097), not to the output path.
- **A per-render byte budget**: it needs counting on every allocation. 0262's per-operation caps
  plus an output limit bound untrusted templates cheaply; a host that wants a real budget enforces
  it in its allocator and throws `jinja2::Exception{ResourceLimitExceeded}` (Q5).
- **`RenderTo(fmt::memory_buffer&)`** (or any fmt type in a signature): it makes fmt a public
  dependency, ties the 2.x ABI to the fmt pin, clashes with a host's own fmt (Conan/system), and
  blocks replacing fmt later as 0230 does Boost. `ContainerSink` covers it header-only.
- **Switching `Render(ostream&)` to streaming**: a silent change in what a failed render leaves in a
  file (R3).
- **`ErrorCode::OutOfMemory` in `Result`.** It needs an allocation-free error path in every
  handler, departs from Python, and disagrees with 0250's R2. Template-driven oversized requests
  are `Result` errors already, through the caps.
- **`noexcept` on Load/Render**: they propagate `bad_alloc` and run host code (Q5).

## 10. Decisions for Ruslan (recommendation first)

- **R1. Contract shape: A** (no allocator parameters in 2.0; contract, survivable OOM,
  `RenderTo`, `ReleaseThreadCaches`), over B or C. Why: section 2. Hooks remain addable in 2.x
  without breaking ABI.
- **R2 (= 0250's D5, approved in #584). OOM: `std::bad_alloc` propagates from every call, for real
  system OOM only** (rec.), after T1 makes the library survive it, over an `ErrorCode::OutOfMemory`
  `Result`. Every limit and spent budget (the library's, the arena's `length_error`, or the host's
  via `jinja2::Exception{ResourceLimitExceeded}`) is `ResourceLimitExceeded` in the `Result`. Why: Python's `MemoryError`;
  no error object built after a failed allocation; template-driven sizes already fail as `Result`
  errors. One answer serves both plans.
- **R3. `Render(ostream&)` stays all-or-nothing** (rec.), over streaming through a sink with
  partial output on error, as Python's `generate()` does. Why: no silent change for file writers.
  Streaming is one sink away.
- **R4. Output API as `RenderTo` overloads** (rec.), over a `BasicRenderOutput` target-handle type
  or a `RenderOptions` struct. Why: each new target is ABI-additive, with no raw pointer and no
  handle layout to freeze. Cost: 4 declarations instead of 2.
