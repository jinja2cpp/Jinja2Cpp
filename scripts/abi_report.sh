#!/usr/bin/env bash
# Compares the library's ABI with the previous one for one C++ standard (docs/tasks/0076).
#   scripts/abi_report.sh <std>      # after abidw wrote .abi/libjinja2cpp.so.2.cxx<std>.abi
# Run by the abidiff steps of .github/workflows/linux-build.yml.
#
# Until the 2.0 RC no baseline is committed and the dump of the last master build
# (.abi-base/, restored from the Actions cache) is the reference: the report shows what a
# pull request changes and never fails. From the RC on, abi/libjinja2cpp.so.<N>.cxx<std>.abi
# is the reference, and an incompatible change fails unless the library's
# SONAME differs from the baseline's, that is unless SOVERSION was bumped (in which case the
# pull request also replaces the baseline with the new dump). An incompatible change is any
# change left once added symbols are ignored: removed or changed functions and variables,
# and changed layouts of the types they use.
set -uo pipefail

std=${1:?usage: abi_report.sh <std>}
current=$(ls .abi/libjinja2cpp.so.*.cxx"$std".abi 2>/dev/null | head -n 1)
baseline=$(ls abi/libjinja2cpp.so.*.cxx"$std".abi 2>/dev/null | head -n 1)
mode=gate
if [ -z "$baseline" ]; then
    mode=report
    baseline=$(ls .abi-base/libjinja2cpp.so.*.cxx"$std".abi 2>/dev/null | head -n 1)
fi
summary=${GITHUB_STEP_SUMMARY:-/dev/null}

if [ -z "$current" ]; then
    echo "::error::no ABI dump for C++$std in .abi/"
    exit 1
fi
if [ -z "$baseline" ]; then
    echo "No previous ABI dump for C++$std yet: nothing to compare." | tee -a "$summary"
    exit 0
fi

abidiff --suppressions abi/suppressions.ini --no-show-locs "$baseline" "$current" > .abi/abidiff.txt 2>&1
rc=$?
cat .abi/abidiff.txt
# Without the added symbols, any remaining change is a break: abidiff sets bit 8 only for
# removed symbols and vtable changes, and reports a changed layout (a new Settings field, a
# Value alternative, a widened member) with bit 4 alone
abidiff --suppressions abi/suppressions.ini --no-show-locs --no-added-syms "$baseline" "$current" > /dev/null 2>&1
break_rc=$?

soname() { grep -o -m 1 "soname='[^']*'" "$1" | cut -d"'" -f2; }
verdict="no ABI change"
if (( (rc | break_rc) & 3 )); then
    verdict="abidiff failed (exit $rc, $break_rc)"
elif (( break_rc & 12 )); then
    verdict="incompatible ABI change"
elif (( rc & 4 )); then
    verdict="compatible ABI change (additions)"
fi

{
    echo "### ABI, C++$std: $verdict"
    echo
    echo "Compared with \`$baseline\` ($mode mode)."
    if (( rc != 0 )); then
        echo
        echo '<details><summary>abidiff report</summary>'
        echo
        echo '```'
        head -n 400 .abi/abidiff.txt
        echo '```'
        echo '</details>'
    fi
} >> "$summary"

if [ "$mode" = report ]; then
    if (( (rc | break_rc) & 3 )); then
        echo "::warning::$verdict; report mode does not fail"
    elif (( break_rc & 12 )); then
        echo "::notice::C++$std: $verdict against the last master build (report only until the 2.0 RC)"
    fi
    exit 0
fi
if (( (rc | break_rc) & 3 )); then
    echo "::error::$verdict"
    exit 1
fi
if (( break_rc & 12 )); then
    if [ "$(soname "$baseline")" != "$(soname "$current")" ]; then
        echo "::notice::C++$std: $verdict with SONAME $(soname "$current") (was $(soname "$baseline")); commit the new dump as the baseline"
        exit 0
    fi
    echo "::error::C++$std: $verdict without a SOVERSION bump. Bump SOVERSION in CMakeLists.txt and commit the job's dump (artifact abi-cxx$std) as abi/, or keep the ABI (docs/tasks/0076)."
    exit 1
fi
exit 0
