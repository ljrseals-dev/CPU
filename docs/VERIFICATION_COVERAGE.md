# Neuro-Silica instruction verification coverage

**Status:** AUTHORED — NOT BUILD-VERIFIED  
**Implementation under test:** `ljrseals-dev/CPU`  
**Reference baseline:** `ljrseals-dev/Silica-Allah` `main` at `92b4e6751111a03b83080d773ed3b1b34a208d01`

This ledger deliberately separates implementation from evidence. A source-level decoder match is not a verified instruction implementation.

| Instruction / behavior | Source implementation | Native known-answer test | Differential test | Mutation test | Formal proof |
|---|---|---|---|---|---|
| MOVZ (64-bit) | Present | Existing differential probe only; independent test pending | Fixed probe | Pending | None |
| ADD (shifted-register subset) | Present | Existing differential probe only | Fixed probe | Pending | None |
| SUBS (immediate subset) | Present | Existing differential probe only | Fixed probe | Pending | None |
| B.cond | Present | Existing branch smoke test | Fixed probe covers taken EQ only | Pending | None |
| B (unconditional) | Present | Pending | Pending | Pending | None |
| LDR/STR (64-bit unsigned offset) | Present | Bounds smoke test; success-path test in differential probe | Fixed probe | Pending | None |
| LDR/STR (selected 64-bit register offset) | Present | Pending | Pending | Pending | None |
| RET | Present | Pending | Pending | Pending | None |
| MADD (64-bit, including XZR accumulator) | Authored on this branch | Regression tests authored; execution pending | Pending | Deterministic decode mutant authored; execution pending | None |
| SVC host-yield convention | Private downstream extension | Existing host/critic tests; semantics are not A64 exception conformance | Excluded | Pending | None |
| Unsupported opcode / sticky fault | Present | Partial | Different reference halt contract; excluded | Pending | None |
| Alignment and memory bounds | Present | Existing bounds/alignment smoke tests | Intentionally excluded from equivalence suite | Pending | None |

## Status vocabulary

- **Present:** source contains an implementation path; correctness is not implied.
- **Authored:** test or verification artifact exists but has not been executed.
- **Verified:** command was executed against the recorded source/toolchain and its result is retained.
- **Excluded:** intentionally outside the declared comparison contract.
- **Pending:** no adequate test or proof exists yet.

## Expansion rule

Do not mark a row verified because a decoder pattern exists, because a reference simulator supports the instruction, or because a broad test passes. Each verified row needs an independent expected result and/or a differential case, with operand boundaries and failure behavior recorded.
