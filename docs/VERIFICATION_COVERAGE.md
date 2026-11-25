# Neuro-Silica instruction verification coverage

**Status:** SOURCE-REVIEWED LEDGER — NOT A FORMAL PROOF  
**Implementation under test:** ljrseals-dev/CPU  
**Reference baseline:** ljrseals-dev/Silica-Allah commit 92b4e6751111a03b83080d773ed3b1b34a208d01  
**CI evidence snapshot:** GitHub Actions run #32 passed GCC, Clang, WASM, Node, sanitizer/mutation target, and pinned-reference differential jobs for the MADD mutation port. This is test evidence for that snapshot, not proof of complete instruction-set conformance.

This ledger separates source implementation from evidence. A decoder match is not, by itself, a verified instruction implementation.

| Instruction / behavior | Source implementation | Native known-answer coverage | Differential coverage | Mutation coverage | Formal proof |
|---|---|---|---|---|---|
| MOVZ (64-bit) | Present | Indirect via instruction-sequence tests | Fixed probe | Pending | None |
| ADD (shifted-register subset) | Present: W/X LSL, LSR, ASR | W/X shift cases and invalid encodings | Fixed probe includes shifted forms | Pending | None |
| SUBS (immediate subset) | Present; Rn=31 reads SP | SP/NZCV regression | Fixed probe | Pending | None |
| B.cond | Present: 15 defined conditions | All 15 conditions × all 16 NZCV states | EQ path in fixed probe | Pending | None |
| B / BL (immediate subset) | Present; BL writes X30 | BL target and link-register regression | BL and B control-flow sequence | BL mutation | None |
| LDR/STR (64-bit unsigned offset) | Present with checked address addition | Bounds and overflow faults; success path in probe | Fixed probe | Pending | None |
| LDR/STR (selected 64-bit register offset) | Present with checked scaling/addition | Overflow-fault regression; broader success matrix pending | Not in current fixed trace | Pending | None |
| RET Xn | Present | RET X3 regression | RET X30 in fixed probe | Pending | None |
| MADD (W/X forms) | Present | W/X accumulator and zero-register cases | W/X forms in fixed probe | MADD dispatch mutant killed in CI | None |
| MSUB (W/X forms) | Present | W/X arithmetic regressions | W/X forms in fixed probe | Pending | None |
| SVC host-yield convention | Private downstream extension | Host/critic harness and PC-continuation regression | Excluded from A64 equivalence | Pending | None |
| Unsupported opcode / sticky fault | Present | Fault and PC rollback regression | Different halt contract; excluded | Pending | None |
| Alignment and memory bounds | Present | Alignment/bounds regressions | Intentionally excluded from equivalence suite | Pending | None |

## Status vocabulary

- **Present:** source contains an implementation path; correctness is not implied.
- **Covered:** an explicit native test or differential sequence exercises the behavior.
- **Verified by CI:** the named workflow snapshot completed successfully; scope is limited to the executed cases and toolchain.
- **Excluded:** intentionally outside the declared comparison contract.
- **Pending:** no adequate independent test, mutation, or proof exists yet.
- **Formal proof:** a proof tied to the actual C++ implementation and its execution semantics; none is claimed in this ledger.

## Expansion rule

Do not mark a row verified because a decoder pattern exists, because the reference simulator supports the instruction, or because a broad test passes. Each row needs independent expected results and/or a differential case, operand boundaries, and failure behavior. Mutation survivors must be investigated; equivalent mutants must be justified rather than counted as killed.
