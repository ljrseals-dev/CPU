# CPU Correctness Integration Audit — 2026-11-24

**Author:** Lewis James Seals Jr. (@ljrseals-dev)
**Status:** SOURCE-AUDITED — NOT BUILD-VERIFIED
**Branch:** `audit/integration-cpu-correctness-20261124`
**Baseline:** current `main` after PR #14 (Corrected Baseline Bug Fix)

## Scope

This document compares the source patches in PRs #10–#14 and records the changes that must be carried forward into a clean integration. It does not claim that tests were run or that any emulator conformance is established.

## PR disposition

- **#10 — control flow, address overflow, MEMT1 fallback:** retain unique fixes. It introduces `add_address`, checks index scaling, general `RET Xn`, `BL` link-register writes, correct `SUBS` SP-source behavior, safer MEMT1 range validation, and read/compute-before-write behavior. Its patch is the foundational correctness slice.
- **#11 — shifted-register ADD:** retain the 32-/64-bit LSL, LSR, and ASR forms, W-register truncation/zero-extension, and reserved-form tests. Review signed ASR portability and instruction masks before integration.
- **#12 — faulting PC:** retain fault-PC rollback for unsupported instructions and data-access faults, while preserving the prototype SVC yield continuation behavior. Review the coverage of every fault path after integration.
- **#13 — W-register MADD/MSUB:** retain 32-bit forms with low-word operands and zero-extended W results. Keep tests with nonzero upper halves.
- **#14 — corrected baseline:** useful readability/refactoring and ESM re-export, but it is not a replacement for #10–#13. The source rewrite visibly drops several #10 semantics.

## Confirmed regression gaps in current `main` source

Source inspection of current `main` shows:

1. `BL` does not write X30.
2. Decoder accepts only conventional `RET X30`, not arbitrary `RET Xn`.
3. `SUBS (immediate)` reads `XZR` for Rn=31 instead of SP, and does not write the encoded destination.
4. Effective-address additions for memory accesses can wrap before bounds checking.
5. ADD shifted-register semantics are limited to the unshifted 64-bit form.
6. 32-bit W-register MADD/MSUB are absent.
7. PC is advanced before decode/execute and is not restored for execution faults.

These are source-inspection findings, not runtime test results.

## Integration progress

- **AUTHORED:** consolidated `src/silica/cpu.cpp` with the fault-PC rollback baseline, checked address arithmetic, BL/RET Xn, SUBS SP-source semantics, shifted ADD (W/X), and W/X MADD/MSUB.
- **AUTHORED:** added W-register MADD/MSUB native regression tests using operands with nonzero upper halves.
- **NOT YET CONSOLIDATED:** MEMT1 fallback safe-snapshot/input validation changes, W-form instructions in the differential probe, and full CI workflow review.
- **NOT BUILD-VERIFIED:** no local compiler/test run has occurred.


## Integration plan

1. Keep this branch based on current `main`; do not rewrite or force-push existing PR branches.
2. Consolidate the unique #10–#13 source and tests into one coherent implementation, resolving `cpu.cpp` once.
3. Review decoder masks and W/X semantics against the pinned Silica-Allah reference.
4. Run the complete `make test` locally only in a verified toolchain environment; separately record actual GitHub Actions evidence.
5. Add missing regression cases discovered during review before treating CI as sufficient.
6. Open a single integration PR with the original PRs cross-linked; close/retire superseded PRs only after their unique changes are preserved.

## Additional source-review notes

- #12's fault rollback should be checked against every faulting path, including instruction fetch faults and any future decoder branch. Fetch faults leave PC unchanged because PC is not incremented until after a successful fetch.
- #11's ASR implementation uses signed right shift of negative values, which is implementation-defined in C++17; integration should document the supported compiler behavior or implement a portable arithmetic shift explicitly.
- PR #14 rewrites SUBS with `a = rr(rn)` and `wr(rd, res)`. That reintroduces the Rn=31/SP issue (because `rr(31)` is XZR) and changes SUBS destination behavior compared with #10. Keep #10's correct `base(rn)` source handling and flags-only SUBS behavior unless the ISA subset/spec explicitly requires a destination write.
- PR #14's memory addressing adds unchecked arithmetic and thereby drops #10's overflow protections.
- PR #14's RET decoder only recognizes the fixed X30 encoding, dropping #10's general RET Xn support.
- PR #14's B decoder treats BL as B and does not write X30, dropping #10's link semantics.

## Verification status

**No build, native test, Node test, WASM build, mutation run, or differential run was performed in this audit step.** GitHub source/patch inspection only. Existing CI pass statements in PR descriptions are historical claims and were not independently re-run here.

## Consolidation status update

The integration branch now contains authored CPU decoder changes and native tests. It is intentionally not yet a complete consolidation of PR #10–#13: MEMT1 fallback source/tests and the differential probe remain outstanding. The source-level integration is committed, but must be reviewed and built before opening a merge-ready PR.


## Further consolidation — MEMT1 and verification harness

- **AUTHORED:** imported PR #10's `host/memt1_fallback.mjs` range validation, 8-byte alignment checks, U64 validation, full input snapshot, output staging, and callback compatibility adapter.
- **AUTHORED:** imported MEMT1 tests for fixed-point behavior, wraparound, invalid values, alignment/range rejection, and no writes after a read failure.
- **AUTHORED:** updated the differential probe to include a W-register shifted ADD and W-register MADD/MSUB with high bits set in source X registers, to expose accidental 64-bit operand use.
- **AUTHORED:** added Makefile sanitizer and mutation targets and configured native CI to run the sanitizer target.
- **REVIEW NOTE:** MEMT1 stages all reads and calculations before writing, but a write callback that itself fails partway through can still leave partial writes. The API has no transactional write primitive; do not describe it as fully failure-atomic for write failures.
- **NOT VERIFIED:** no native/WASM build, Node test, sanitizer, mutation, or differential execution has been run by the assistant. A draft PR will be used to let repository CI evaluate the branch.

## Remaining review risks

1. Verify the differential probe against the pinned Silica-Allah revision; do not assume parity from matching source intent.
2. Review the exact instruction encodings and expected W-register values in the differential trace.
3. Check the host adapter's old callback argument mapping and ESM import paths in the full project.
4. Validate the mutation script and ensure the sanitizer target is compatible with both GCC and Clang runners.
