# Metronome-inspired verification adaptation for Neuro-Silica

**Status:** RESEARCH ADAPTATION — IMPLEMENTATION AND EVIDENCE EVOLVING  
**Author:** Lewis James Seals Jr. (ljrseals-dev)  
**Copyright:** Copyright (c) 2026 Lewis James Seals Jr.  
**SPDX-License-Identifier:** MIT

## Research source and scope

Research reference: brentius/metronome, branch metronome-v4.0.

The verification ideas adapted here are per-step machine lockstep and normalized state dumps; mutation testing of implementation logic and verification artifacts; specification-clause-to-model-declaration traceability; and per-module refinement proofs composed into larger transition-system claims.

Metronome is a hardware/RTL project. Its Yosys extraction, RTL cuts, physical-design checks, and chip-specific proofs do not directly apply to this C++ instruction-set simulator. This document adapts verification methods, not Metronome implementation code.

## CPU-specific adaptation

1. Keep Silica-Allah and Neuro-Silica as separate executables. Both use namespace silica; do not combine their headers or link both engines into one process for comparison.
2. Pin the reference checkout to Silica-Allah commit 92b4e6751111a03b83080d773ed3b1b34a208d01 for reproducible differential runs.
3. Compare after every executed instruction: PC, SP, NZCV, X0-X30, touched memory, and execution/fault outcome where contracts overlap.
4. Keep intentionally different contracts (SVC host yield, alignment, memory sizing, unsupported-instruction halt semantics) in an expected-divergence suite.
5. Use independent known-answer tests so the differential oracle is not the only source of expected behavior.
6. Mutate implementation logic and require the corresponding regression to fail. Report killed, surviving, and equivalent mutants separately.
7. Maintain an instruction coverage ledger that distinguishes implementation, executed test evidence, differential evidence, and formal proof.

## Historical MADD dispatch finding

The original Neuro-Silica decoder used a masked comparison whose constant could not match after masking, and the implementation covered only a narrow X-form multiply case. The original branch corrected the dispatch and added accumulator and XZR-accumulator regressions.

The maintained implementation has since evolved beyond that historical patch: the current decoder supports 32-bit and 64-bit MADD/MSUB forms, masks W operands to 32 bits, zero-extends W results, and uses the architectural accumulator operand. Current native tests cover W/X MADD/MSUB; the maintained mutation suite includes a deliberate MADD dispatch mutant.

Do not treat this historical finding as a statement that MSUB remains unsupported.

## Verification approach

The maintained targets are: make clean; make test; make san; make differential SILICA_ALLAH_DIR=../Silica-Allah; and make mutation.

The current CI workflow runs native GCC and Clang builds, WASM linking, Node tests, sanitizer checks, mutation checks, and the pinned-reference differential trace. GitHub Actions run #32 passed those jobs on the MADD mutation port pull request. This is evidence for the tested source snapshot and test cases, not a blanket correctness claim.

## Proof boundary

A future Lean model may specify arithmetic, flags, branches, and memory address calculations. A proof about that model alone does not prove this C++ implementation. A formal conformance claim requires a reviewed connection between the C++ execution path and the formal semantics, in addition to independent known-answer, mutation, and differential evidence.
