#!/bin/bash
# SessionStart hook for Claude Code on the web: makes `cmake`/`ctest` work out of
# the box in a fresh cloud container and leaves a warm Debug build in build/.
set -euo pipefail

if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
  exit 0
fi

cd "$CLAUDE_PROJECT_DIR"

# Toolchain bits the base image may lack: Ninja, and the clang sanitizer
# runtimes needed for -DJINJA2CPP_WITH_SANITIZERS=address+undefined with clang.
missing=()
command -v ninja >/dev/null || missing+=(ninja-build)
command -v clang-format >/dev/null || missing+=(clang-format)
clang_major=$(clang++ --version 2>/dev/null | sed -n 's/.*clang version \([0-9]*\).*/\1/p' | head -1)
if [ -n "$clang_major" ] && ! dpkg -s "libclang-rt-${clang_major}-dev" >/dev/null 2>&1; then
  missing+=("libclang-rt-${clang_major}-dev")
fi
if [ ${#missing[@]} -gt 0 ]; then
  (sudo -n true 2>/dev/null && SUDO=sudo || SUDO=; \
   $SUDO apt-get update -qq && $SUDO apt-get install -y -qq --no-install-recommends "${missing[@]}") \
    || echo "session-start: could not install ${missing[*]}; continuing" >&2
fi

# GitHub archive tarballs are blocked by the cloud egress proxy; clone the pinned
# dependencies with git instead and point FetchContent at the clones.
deps_cache="${HOME}/.cache/jinja2cpp-deps"
init_cache=$(python3 .claude/hooks/prefetch_deps.py "$CLAUDE_PROJECT_DIR" "$deps_cache")
echo "export JINJA2CPP_CMAKE_INIT=\"$init_cache\"" >> "$CLAUDE_ENV_FILE"

# Warm Debug build so the first edit-build-test cycle is incremental.
cmake -S . -B build -G Ninja -C "$init_cache" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DFETCHCONTENT_BASE_DIR="${deps_cache}/fetchcontent" >/dev/null
cmake --build build --parallel
