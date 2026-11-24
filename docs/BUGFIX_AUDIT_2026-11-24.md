# Neuro-Silica focused bug-fix audit — 2026-11-24

**Author:** Lewis James Seals Jr. (ljrseals-dev)  
**License:** MIT  
**Status:** Source changes authored; this branch's CI verification pending.

## Confirmed defects addressed in this change

1. **BL decoded as B without setting X30.** The shared B/BL encoding path now detects the link bit and writes the sequential return address to X30. A native regression checks BL target, LR, and RET behavior.
2. **RET was hard-coded to RET X30.** The decoder now accepts the general RET Xn encoding; the native regression uses RET X3.
3. **SUBS immediate read XZR instead of SP when Rn=31.** This encoding uses SP as its base operand; the decoder now reads SP while Rd=31 still discards the result as XZR.
4. **Effective-address addition could wrap modulo 2^64.** Unsigned-offset and register-offset LDR/STR paths now detect overflow before memory access. Regression cases verify a wrapped store cannot corrupt address zero.
5. **Register-offset scaling could overflow before address validation.** The index is checked before shifting left by three.
6. **MEMT1 fallback accepted unaligned and unsafe address ranges.** Matrix/key/value regions are checked for 8-byte alignment and safe-integer range before reads or writes.
7. **MEMT1 fallback could partially mutate the matrix if a later input read failed.** All inputs are now snapshotted and all outputs computed before the first matrix write.

## Added regression evidence

- BL writes X30 and branches to the encoded target.
- RET X3 transfers control through the selected register.
- SUBS XZR, SP, #1 uses SP and produces the expected NZCV.
- Unsigned and register-offset store overflow faults rather than wrapping to address zero.
- MEMT1 fallback rejects unaligned and unsafe address ranges before memory callbacks.
- MEMT1 fallback performs no writes when an input read fails.
- A mutation target removes BL's X30 write and requires the native regression suite to kill that mutant.
- The pinned-reference differential program now includes SUBS with SP, BL, and RET X30 in its per-instruction state trace. This new extension awaits the pull request's CI result.

## Verification boundaries

- The branch is based on the latest main after PR #8 (native/WASM/Node matrix) and PR #9 (pinned Silica-Allah differential gate).
- This authoring session could not clone the repository because the execution environment could not resolve github.com. No local compiler or Node command ran here.
- The existing CI workflow is expected to execute native, WASM, Node, mutation, and differential checks on this pull request. Its results must be inspected before claiming these new changes pass.
- A successful differential trace proves agreement only for the exact instructions and states exercised by that trace. It does not establish complete AArch64 conformance.
- Unsupported instructions remain unsupported. This is not a complete AArch64 emulator, and no CPU/GPU equivalence or production-readiness claim is made.
