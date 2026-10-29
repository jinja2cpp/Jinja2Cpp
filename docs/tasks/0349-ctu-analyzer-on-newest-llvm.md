---
status: open
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/696
priority: low
area: ci
depends: [0345]
touches: [.github/workflows/clang-analysis.yml]
---
# Run the cross-TU static analyzer on the newest LLVM and on libstdc++

**Problem.** The report-only job of 0345 runs Lifetime Safety and thread safety on LLVM 23.1.3,
but its cross-TU static analyzer runs on LLVM 22.1.8 with libc++ headers, because neither newer
nor libstdc++ setup can import our units:
- LLVM 23.1.3's AST importer rejects every explicit instantiation: "cannot import unsupported AST
  node ExplicitInstantiation", 57 times when `src/template_env.cpp` imports from
  `src/template.cpp` (`template class BasicTemplate<char>;`, and libstdc++'s and libc++'s
  `extern template`s). The unit then fails and writes no SARIF.
- LLVM 22.1.8 (and 23) reject the file-scope `__asm` in libstdc++ 13's `<iostream>`
  ("cannot import unsupported AST node FileScopeAsm"), so the analyzer runs on libc++ 22 headers.
  The rest of CI builds against libstdc++, so a finding that depends on the library's code is
  seen through a different implementation.

**Proposal.** When nixpkgs carries an LLVM whose `ASTImporter` handles `ExplicitInstantiation`
(check its release notes or run `scripts/clang_analysis.py --ctu` on `src/template_env.cpp` and
`src/template.cpp`), move `ctuClang`/`ctuLibcxx` in the workflow to that version, and try
libstdc++ again by dropping the `--ctu-arg` flags. Report the importer gaps upstream if no
issue exists.

**Done when** the cross-TU pass runs on the same LLVM as the compile pass, every unit of `src/`
analyses without import errors, and the choice of standard library is recorded in the workflow.
