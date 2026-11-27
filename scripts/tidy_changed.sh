#!/usr/bin/env bash
# clang-tidy on every C++ file a change touches, the whole file (docs/tasks/0601).
#
# The pull-request job and scripts/preflight.sh both run this, so they cannot drift apart.
# Changed lines were not enough: cognitive complexity and include-cleaner report a function's
# or an #include's own line, and a type change moves hits into callers nobody edited. A file
# a change touches has to be clean as a whole; master is clean (docs/tasks/0600).
#
# Usage: scripts/tidy_changed.sh --base REF [--build DIR] [--clang-tidy PATH] [--log FILE]
#   --base        ref the change is measured against
#   --build       configured build directory with compile_commands.json (default: build)
#   --clang-tidy  the clang-tidy binary (default: clang-tidy)
#   --log         file for the output (default: tidy.log); the output is printed too
#
# A translation unit is analysed with its own flags; a header on its own, borrowing the flags of
# the most similar file in the database, as the whole-tree job in clang-tidy.yml does. The
# generated include directory (jinja2cpp/version.h) is passed explicitly. Exit status: 0 clean,
# non-zero when clang-tidy reports an error (every enabled check is one, see .clang-tidy).
set -uo pipefail

base="" build=build tidy=clang-tidy log=tidy.log
while [ $# -gt 0 ]; do
  case "$1" in
    --base) base=$2; shift 2 ;;
    --build) build=$2; shift 2 ;;
    --clang-tidy) tidy=$2; shift 2 ;;
    --log) log=$2; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done
[ -n "$base" ] || { echo "--base is required" >&2; exit 2; }

# Out: the Boost components and the Boost.JSON binding test, which this configuration gives no
# include path (docs/tasks/0311; the big-integer adapter is analysed through
# test/big_int_test.cpp, 0272); vendored files; generated test data (included by a test); the
# installed-package consumers, built outside this compile database; the 1.x migration fixture,
# which compiles only once migrated (0077)
mapfile -t files < <(git diff --name-only --diff-filter=d "$base" -- src include bindings test \
    ':!bindings/boost_*' ':!test/binding/boost_json*' \
    ':!include/jinja2cpp/polymorphic_value/*' ':!src/ankerl/*' ':!src/lexertk.h' \
    ':!src/unicode_tables.h' ':!test/*.inc' ':!test/package/*' ':!test/migration/*' \
  | grep -E '\.(cpp|h)$' || true)

: > "$log"
if [ ${#files[@]} -eq 0 ]; then
  echo "clang-tidy: no changed C++ files"
  exit 0
fi
echo "clang-tidy on ${#files[@]} changed file(s), whole files"
printf '%s\n' "${files[@]}" \
  | xargs -P "$(nproc)" -n 1 "$tidy" -p "$build" --quiet \
      --extra-arg=-Wno-unknown-warning-option \
      --extra-arg=-I"$(realpath "$build")/generated/include" \
  > "$log" 2>&1
status=$?
cat "$log"
exit "$status"
