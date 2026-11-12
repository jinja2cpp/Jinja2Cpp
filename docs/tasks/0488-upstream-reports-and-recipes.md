---
status: open
priority: low
area: release
depends: [0279]
touches: []
shares: [docs/dependencies.md]
---
# Report our fixes upstream and update the package recipes

**Problem.** Some of what we fixed or worked around lives only in this repository. Each one
we carry costs a re-check on every dependency bump, and other users of those projects keep
the defect. None of this work changes the repository's code. It is issues and pull requests
in other projects, filed by Ruslan or with his approval: they are posted under his name.

**Work.**
1. **nlohmann/json.** `~basic_json` is `noexcept`, and `json_value::destroy` (v3.12.0)
   reserves a heap `std::vector` to flatten a non-empty array or object, so an allocation
   failure while a document is destroyed calls `std::terminate` (0279; our side of the fix is
   0487). File the issue from the draft in the project files
   (`0279/nlohmann-upstream-issue-draft.md`), with the allocation-free drain as the proposed fix.
2. **jbcoe/value_types.** Our vendored copy at 57f2e7a
   (`include/jinja2cpp/polymorphic_value/`) has fixes upstream may want:
   - the `in_place_type_t<U>`, `initializer_list` constructor of `polymorphic` creates `U`,
     where upstream creates `T`, which slices, or fails to compile for an abstract `T`;
   - `cb_` has a default member initialiser, which silences GCC `-O2` `-Wmaybe-uninitialized`
     (0089);
   - the default constructor is constrained on the allocator being default constructible, for
     MSVC's `std::variant` (0071);
   - `indirect` tests the tag first, so overload resolution never asks whether an incomplete
     `T` is constructible (0072).
   First check upstream's current head: report only what it still has, with a reproducer each.
   The namespace move (0076) is ours alone.
3. **Package recipes.** Update the ConanCenter recipe (`recipes/jinja2cpp` in
   conan-io/conan-center-index) and the vcpkg port to 2.0: the new version, the dependency set
   after 0230 (Boost optional), the JSON binding components (0311), and the license metadata
   0226 corrects. This is step 4 of 0006 and the recipe correction of 0226, which this task
   carries out once 2.0 is tagged.

**Done when.** Each item has an upstream link (issue, PR or merged commit) recorded here, and
docs/dependencies.md notes any local change that upstream has taken, so the next bump can drop
it.
