# Neuro-Silica focused bug-fix audit — 2026-11-24

**Author:** Lewis James Seals Jr. (ljrseals-dev)  
**License:** MIT  
**Status:** VERIFIED by GitHub Actions run #15 — GCC, Clang, ASan/UBSan, Node, WASM, mutation, and pinned-reference differential jobs passed.

## Confirmed defects addressed in this change

1. **BL decoded as B without setting X30.** The shared B/BL encoding path now detects the link bit and writes the sequential return address to X30. A native regression checks BL target, LR, and RET behavior.
2. **RET was hard-coded to RET X30.** The decoder now accepts the general RET Xn encoding; the native regression uses RET X3.
3. **SUBS immediate read XZR instead of SP when Rn=31.** This encoding uses SP as its base operand; the decoder now reads SP while Rd=31 still discards the result as XZR.
4. **Effective-address addition could wrap modulo 2^64.** Unsigned-offset and register-offset LDR/STR paths now detect overflow before memory access. Regression cases verify a wrapped store cannot corrupt address zero.
5. **Register-offset scaling could overflow before address validation.** The index is checked before shifting left by three.
6. **MEMT1 fallback accepted unaligned and unsafe address ranges.** Matrix/key/value regions are checked for 8-byte alignment and safe-integer range before reads or writes.
7. **MEMT1 fallback could partially mutate the matrix if a later input read failed.** All inputs are now snapshotted and all outputs computed before the first matrix write.

8. **B.cond coverage was too narrow.** Added exhaustive checks of all 15 defined condition codes against all 16 NZCV combinations.

## Added regression evidence

- BL writes X30 and branches to the encoded target.
- RET X3 transfers control through the selected register.
- SUBS XZR, SP, #1 uses SP and produces the expected NZCV.
- Unsigned and register-offset store overflow faults rather than wrapping to address zero.
- MEMT1 fallback rejects unaligned and unsafe address ranges before memory callbacks.
- MEMT1 fallback performs no writes when an input read fails.
- A mutation target removes BL's X30 write and requires the native regression suite to kill that mutant.
- Every defined B.cond predicate is checked over all 16 NZCV states.
- Added an ASan/UBSan native test target and wired it into the GCC/Clang CI matrix.
- The pinned-reference differential program includes SUBS with SP, BL, RET X30, and the continuation instruction; the expanded trace passed in GitHub Actions run #15.

## Verification boundaries

- The branch is based on the latest main after PR #8 (native/WASM/Node matrix) and PR #9 (pinned Silica-Allah differential gate).
- This authoring session could not clone the repository because the execution environment could not resolve github.com; validation therefore ran on GitHub-hosted runners rather than this local environment.
- GitHub Actions run #15 passed the complete native/Node/WASM/mutation suite under both GCC and Clang, and passed the expanded pinned Silica-Allah differential trace.
- The native regression suite also passed under AddressSanitizer and UndefinedBehaviorSanitizer with both GCC and Clang.
- A successful differential trace proves agreement only for the exact instructions and states exercised by that trace. It does not establish complete AArch64 conformance.
- Unsupported instructions remain unsupported. This is not a complete AArch64 emulator, and no CPU/GPU equivalence or production-readiness claim is made.


## Follow-up audit — ADD (shifted register)

**Finding:** The previous decoder only matched the 64-bit ADD register encoding with a zero shift amount. It did not implement the general A64 ADD (shifted register) form even though that instruction family is part of the pinned reference's supported subset. This was a coverage gap, not a claim that nonzero shifts were silently executed incorrectly.

**Change authored on branch `fix/add-shifted-register-semantics-20261124`:**
- Decode 32-bit and 64-bit ADD (shifted register) forms.
- Implement LSL, LSR, and ASR using the encoded shift type and amount.
- Preserve W-register zero-extension and XZR behavior.
- Reject reserved ROR shift type and out-of-range 32-bit shift amounts as unsupported.
- Add native regressions for 64-bit LSL/LSR/ASR, 32-bit LSL/LSR, and invalid encodings.
- Extend the pinned Silica-Allah per-instruction state trace with the same operations and a negative ASR operand.

**Verification status:** AUTHORED — CI NOT YET RUN for this follow-up branch. The earlier run #15 does not cover these new changes and is not evidence for them.
