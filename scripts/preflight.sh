#!/usr/bin/env bash
# Run, before a push, the pull-request checks that a cloud session can run itself, in the
# form CI runs them: clang-format (whole src/ and include/ files, changed lines in test/),
# clang-tidy and the null-comparison query on changed lines (headers analysed on their
# own), the task files, a pattern CodeQL flags, and with --perf the instruction-count gate.
# Twelve of the 19 red-push causes in PRs #346-#445 were one of these checks run locally
# in a narrower form (docs/tasks/0157). What stays out: MSVC, Apple Clang, sanitizers and
# the test suite itself (the verifier and CI cover those).
#
# Usage: scripts/preflight.sh [--perf] [--build DIR] [BASE]
#   BASE     ref to compare with (default: merge base with origin/master)
#   --build  configured build directory with compile_commands.json (default: build)
#   --perf   also build Release benchmarks of HEAD and BASE and run the CI gate
#            (bench/count.py --threshold 0.03); takes several minutes
set -uo pipefail

cd "$(git rev-parse --show-toplevel)"
perf=0 build=build base=
while [ $# -gt 0 ]; do
  case "$1" in
    --perf) perf=1 ;;
    --build) build=$2; shift ;;
    -h|--help) sed -n '2,15p' "$0"; exit 0 ;;
    *) base=$1 ;;
  esac
  shift
done
base=$(git merge-base HEAD "${base:-origin/master}") || exit 2

results=() failed=0
record() { results+=("$1  $2"); [ "$1" = FAIL ] && failed=1; }
changed() { git diff --name-only --diff-filter=d "$base" -- "$@"; }
have() { command -v "$1" >/dev/null 2>&1; }

fmt=clang-format-18; have $fmt || fmt=clang-format
tidy_excludes=(':!src/binding/rapid_json*' ':!src/binding/nlohmann_json*'
  ':!include/jinja2cpp/binding/rapid_json*' ':!include/jinja2cpp/binding/nlohmann_json*'
  ':!include/jinja2cpp/polymorphic_value/*' ':!src/robin_hood.h' ':!src/lexertk.h'
  ':!src/unicode_tables.h')

# 1. clang-format, as .github/workflows/format-check.yml
mapfile -t whole < <(changed 'src/**.h' 'src/**.cpp' 'include/**.h')
if [ ${#whole[@]} -gt 0 ] && ! $fmt --dry-run --Werror "${whole[@]}" > /tmp/preflight-format.log 2>&1; then
  head -20 /tmp/preflight-format.log
  record FAIL "clang-format: src/include files (run: $fmt -i <file>)"
else
  out=$(git clang-format --binary "$fmt" --diff --extensions cpp,h,hpp "$base" -- test 2>&1)
  if [ -n "$out" ] && ! grep -q -e 'no modified files to format' -e 'did not modify any files' <<<"$out"; then
    echo "$out" | head -40
    record FAIL "clang-format: changed lines in test/ (run: git clang-format $base)"
  else
    record PASS "clang-format"
  fi
fi

# 2. clang-tidy on changed lines, as .github/workflows/clang-tidy.yml
tidy_dir=$(python3 -c 'import clang_tidy, os; print(os.path.join(os.path.dirname(clang_tidy.__file__), "data", "bin"))' 2>/dev/null)
if [ -z "$(changed src include test)" ]; then
  record SKIP "clang-tidy: no C++ changes"
elif [ -z "$tidy_dir" ] || [ ! -f "$build/compile_commands.json" ]; then
  record FAIL "clang-tidy: needs 'pip install clang-tidy==22.1.8' and $build/compile_commands.json"
else
  # A build directory configured before the last CMake change has a stale compile
  # database (missing definitions read as errors in headers); reconfigure it first.
  cmake -S . -B "$build" >/dev/null 2>&1 || record WARN "cmake reconfigure of $build failed"
  git diff -U0 --no-color "$base" -- src include test "${tidy_excludes[@]}" \
    | python3 "$tidy_dir/clang-tidy-diff.py" -p1 -path "$build" -j "$(nproc)" -quiet \
        -clang-tidy-binary "$tidy_dir/clang-tidy" -extra-arg=-Wno-unknown-warning-option \
    > /tmp/preflight-tidy.log 2>&1
  tidy_status=$?
  grep -E ': (warning|error): ' /tmp/preflight-tidy.log | sort -u | head -30
  # CI fails on the exit status (errors and WarningsAsErrors checks), not on plain warnings
  if [ $tidy_status = 0 ]; then
    record PASS "clang-tidy (changed lines, headers on their own)"
  else
    record FAIL "clang-tidy: see /tmp/preflight-tidy.log"
  fi
  query=clang-query-18; have $query || query=clang-query
  if python3 scripts/null_compare.py -p "$build" --changed "$base" --fail --clang-query "$query"; then
    record PASS "null comparisons in conditions"
  else
    record FAIL "null comparisons: scripts/null_compare.py --fix"
  fi
fi

# 3. CodeQL cpp/suspicious-add-sizeof: pointer + sizeof(...) on added lines. A heuristic;
# CodeQL flags it unless the pointer is char-sized, so name the byte offset instead.
hits=$(git diff -U0 "$base" -- src include | grep -E '^\+[^+]' \
  | grep -nE '[]A-Za-z0-9_)] *[-+] *sizeof *\(|sizeof *\([^)]*\) *[-+] *[A-Za-z0-9_(]|[-+] *[A-Za-z0-9_.]+ *\* *sizeof *\(' || true)
if [ -n "$hits" ]; then
  echo "$hits"
  record WARN "CodeQL suspicious-add-sizeof pattern on added lines (check pointer types)"
else
  record PASS "CodeQL sizeof pattern"
fi

# 4. Task files
if [ -n "$(changed docs/tasks)" ]; then
  if python3 scripts/task_batches.py >/dev/null; then record PASS "task files parse"
  else record FAIL "task files: scripts/task_batches.py fails"; fi
  if [ -n "$(changed docs/tasks/README.md)" ] && [ -n "$(changed 'docs/tasks/0*.md')" ] \
     && [ -n "$(changed src include test)" ]; then
    record WARN "code PR edits docs/tasks/README.md: leave the index to scripts/task_index.py"
  fi
fi

# 5. Instruction-count gate, as the instructions job of .github/workflows/benchmark.yml
if [ "$perf" = 1 ]; then
  if [ -z "$(changed src include bench CMakeLists.txt thirdparty)" ]; then
    record SKIP "instruction gate: no src/include/bench changes"
  else
    init=(); [ -n "${JINJA2CPP_CMAKE_INIT:-}" ] && init=(-C "$JINJA2CPP_CMAKE_INIT")
    wt=$(mktemp -d)/base
    git worktree add -f --detach "$wt" "$base" >/dev/null 2>&1
    ok=1
    for side in head base; do
      src=$PWD dir=build-preflight-head
      [ $side = base ] && src=$wt dir=build-preflight-base
      cmake -S "$src" -B "$dir" -G Ninja "${init[@]}" -DCMAKE_BUILD_TYPE=Release \
        -DJINJA2CPP_BUILD_BENCHMARKS=ON \
        "-DCMAKE_CXX_FLAGS=-ffile-prefix-map=$src=S -ffile-prefix-map=$PWD/$dir=B" >/dev/null \
        && cmake --build "$dir" --target jinja2cpp_bench --parallel 2 >/dev/null || ok=0
    done
    if [ $ok = 1 ] \
       && python3 bench/count.py --bench build-preflight-base/bench/jinja2cpp_bench --out /tmp/preflight-base.json >/dev/null \
       && python3 bench/count.py --bench build-preflight-head/bench/jinja2cpp_bench \
            --baseline /tmp/preflight-base.json --threshold 0.03; then
      record PASS "instruction gate (+3% per benchmark)"
    else
      record FAIL "instruction gate or bench build"
    fi
    git worktree remove -f -f "$wt" >/dev/null 2>&1
  fi
fi

echo
echo "preflight against $(git rev-parse --short "$base"):"
printf '  %s\n' "${results[@]}"
exit $failed
