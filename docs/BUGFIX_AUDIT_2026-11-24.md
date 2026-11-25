# Neuro-Silica focused bug-fix audit

**Author:** Lewis James Seals Jr. (ljrseals-dev)  
**License:** MIT  
**Review baseline:** `main` at `d9a168e8ca8e8e545865366c311260b6b83d45e9`  
**Status:** Source-reviewed. CI results were not independently confirmed during this review; no local build or test execution is claimed.

## Implemented behavior observed on the review baseline

1. **BL link register and RET register selection.** The decoder handles B/BL through the shared immediate branch family and writes the return address to X30 for BL. RET decodes its Rn field rather than hard-coding X30.
2. **SUBS SP operand semantics.** SUBS immediate reads SP when Rn is 31; Rd=31 continues to discard the result as XZR.
3. **Checked effective addresses.** Unsigned-offset and register-offset LDR/STR paths check addition overflow before memory access. Register-offset scaling is checked before shifting.
4. **Faulting instruction PC.** Unsupported instructions and failed data accesses restore PC to the faulting instruction. The prototype SVC host-yield path intentionally leaves PC at the following instruction.
5. **MEMT1 input snapshots.** The JavaScript fallback validates aligned, safe address ranges, snapshots all key/value/matrix inputs, computes all outputs, and only then writes the matrix.
6. **B.cond predicate coverage.** Native tests exercise all 15 defined condition codes over all 16 NZCV combinations.
7. **ADD (shifted register).** The decoder handles 32-bit and 64-bit LSL, LSR, and ASR forms, rejects reserved ROR and invalid W-form shift amounts, and preserves W-register zero-extension.
8. **32-bit and 64-bit MADD/MSUB.** The decoder masks W-form operands to 32 bits and zero-extends W-form results.

## Regression coverage observed on the review baseline

- BL target and X30 link value; RET X3 register selection.
- SUBS XZR, SP, #1 and NZCV result.
- Unsigned-offset and register-offset address wraparound faults.
- Fault-PC rollback for unsupported opcodes, bounds faults, and alignment faults.
- SVC host-yield continuation PC.
- MEMT1 rejection of unaligned and unsafe ranges, and no writes after a failed input read.
- Exhaustive B.cond condition/NZCV combinations.
- Shifted ADD variants and invalid encodings.
- W-form and X-form MADD/MSUB.
- The differential probe includes branch/link/return, SP-based SUBS, shifted ADD, and W/X multiply-accumulate cases.

## Verification boundaries

- The source and regression-test files listed above were inspected on the stated `main` baseline.
- The available workflow lookup did not return PR-triggered runs for the reviewed `main` commit. That lookup is not a complete view of all workflow events, so this document does not label main green or red.
- No local compiler, Node, sanitizer, WASM linker, mutation, or differential run was executed as part of this review.
- A passing differential trace, when independently confirmed, establishes agreement only for the exact instructions and states exercised. It does not establish complete AArch64 conformance.
- Unsupported instructions remain unsupported. Neuro-Silica is a limited functional simulator and MEMT1 research prototype, not a complete AArch64 emulator or a production-ready CPU/GPU-equivalence implementation.
