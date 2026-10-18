---
status: open
priority: high
area: robustness
touches: [docs/security.md, SECURITY.md, include/jinja2cpp/filesystem_handler.h]
shares: [docs/parity.md]
---
# Define the template trust model and filesystem-loader boundary

**Problem.** docs/parity.md calls sandboxing inapplicable because templates cannot
access Python objects. Templates still request CPU/memory, invoke host callables,
and use loaders. RealFileSystem::GetFullFilePath does `root /= name`: `../` walks
out of the root, and an absolute name replaces the root entirely (boost::filesystem
`operator/=`). Nothing checks that the resulting path stays under that root.
The name "root" can therefore imply confinement the API does not promise.

**Proposal.** Document the supported trust model and resolve the loader policy:

- Distinguish trusted templates with untrusted data from user-authored templates;
  explain what autoescape protects and how safe/Markup values change that contract.
- State the host's responsibility for exposed callables and loader access. Rendering
  does not make an arbitrary C++ callback safe or preempt a blocking callback.
- Decide whether RealFileSystem remains a trusted-path loader or gains a separately
  designed opt-in confined loader. Review relative traversal, absolute paths,
  platform-specific separators and symlinks before promising confinement.
- Link 0097/0098 for resource limits, current fixed caps and remaining limitations.
  Define the supported containment approach if configurable budgets are deferred.
- Add a security reporting/contact policy and list the versions receiving fixes.

This is a contract and threat-model task, not a commitment to build a complete
sandbox. An API/implementation change to enforce confinement needs its own design,
task and tests. Update the sandbox row in docs/parity.md to link the agreed policy.

**Done when.** A caller can determine whether its template/data/loader/callback
combination is supported, what boundaries are enforced, and what isolation it must
provide. The loader policy is backed by small reproducible probes using disposable
files (including traversal and symlink cases where supported), with no reading of
host-sensitive files. Documentation makes no confinement or resource guarantees
that those probes and the implementation do not support.
