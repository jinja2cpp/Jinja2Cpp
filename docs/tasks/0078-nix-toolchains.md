---
status: open
priority: low
area: ci
depends: [0070]
touches: [flake.nix, flake.lock, .github/workflows/clang-tidy.yml, .github/workflows/linux-build.yml, .claude/hooks/session-start.sh]
---
# Pinned toolchains from Nix for the tool and bleeding-edge rows

**Problem.** The rows that need toolchains newer than the runner's distribution are
assembled by hand: clang-tidy 22 comes from `pip install clang-tidy==22.1.8` (apt has
18), C++26 (0070) needs the newest GCC and Clang, and the cloud session has only GCC 13
and Clang 18. Each row pins its tool its own way, and a local run rarely matches CI.

**Proposal (hybrid, raised by Ruslan 2026-10-02).**
- A `flake.nix` with a pinned `flake.lock` provides dev shells: newest GCC and Clang,
  clang-tidy/clang-format at the versions CI enforces, CMake, Ninja, ccache, Python with
  jinja2 (the parity oracle). `nix develop .#tidy` gives CI, the cloud session and a
  contributor the same binaries.
- Use it in CI only where a pinned or newest tool is the point: the clang-tidy and format
  jobs, the C++26 forward-compatibility row, optionally sanitizers on the newest Clang.
- Keep the compatibility matrix on native compilers: Ubuntu's packaged GCC/Clang
  (including distro combinations such as Clang 18 + libstdc++ 13, which has no
  `std::expected`; a Nix toolchain would have hidden that), MSVC on Windows and Apple
  Clang on macOS. Those are what users build with; Nix cannot provide MSVC or Apple
  Clang at all.

**Costs to measure first.** Nix install plus cache fetch per job (a GitHub Actions Nix
cache action vs. the current apt/pip steps); `cache.nixos.org` and
`releases.nixos.org` answer from the cloud sandbox (checked 2026-10-02), but a store
download through the proxy may be slow; Boost and the other FetchContent dependencies
stay as they are, so the flake only supplies tools.

**Done when.** The tidy/format jobs and the C++26 row run from the flake, the session
hook can enter the same shell, and the README documents `nix develop` as optional.
