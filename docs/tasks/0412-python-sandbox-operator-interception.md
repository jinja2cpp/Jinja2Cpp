---
status: open
priority: low
area: api
depends: [0354]
touches: [src/expression_evaluator.cpp, src/expression_evaluator.h, src/expression_parser.cpp, python/jinja2cpp/sandbox.py, python/src/environment.cpp]
shares: [python/tests/test_sandbox.py, docs/python.md]
---
# Sandbox: `intercepted_binops`, `intercepted_unops`, `call_binop` and `call_unop`

**Problem.** Jinja2's `SandboxedEnvironment` lets a host intercept `+ - * / // ** %` and unary
`+ -` by listing them in `intercepted_binops` / `intercepted_unops`: the compiled template then
calls `environment.call_binop(context, op, left, right)` instead of the operator. It is how a host
stops `'x' * 10**9` or `**` in untrusted templates. The engine has no hook for an operator: a
`BinaryExpression` evaluates its operands and applies the operation. The Python module (0354)
therefore refuses to load a template of an environment with a non-empty set
(`jinja2cpp.UnsupportedError`), because ignoring it would let the operators through. Jinja2's
`test_binary_operator_intercepting` and `test_unary_operator_intercepting` are `xfail` for that.

**Proposal.** Decide with the 0248 owner between (a) a parse-time variant: when the loading
environment has intercepted operators, the parser makes an `InterceptedBinary` / `InterceptedUnary`
node for the listed operators, which evaluates its operands and calls a host callback installed
like `IHostSandbox` (src/host_object.h), so a plain template pays nothing; or (b) leaving it
unsupported and documenting `UntrustedLimits` (`maxSequenceSize`) as the engine's answer to the
same threat. Either way the callback gets Python values (`ToPython`) and its result is read back
with `ToEngine`.

**Done when** the two `xfail` tests in python/tests/test_sandbox.py pass (or are replaced by a
documented refusal with a test) and `docs/python.md`'s sandbox section says which.
