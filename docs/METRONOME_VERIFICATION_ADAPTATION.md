# Metronome-inspired verification adaptation for Neuro-Silica

**Status:** RESEARCH ADAPTATION — IMPLEMENTATION IN PROGRESS  
**Author:** Lewis James Seals Jr. (ljrseals-dev)  
**Copyright:** Copyright (c) 2026 Lewis James Seals Jr.  
**SPDX-License-Identifier:** MIT

## Research source and scope

Research reference: [brentius/metronome](https://github.com/brentius/metronome), branch `metronome-v4.0`. The inspected verification work includes:

- Per-step machine lockstep and normalized state dumps.
- Mutation testing of protocol descriptions, generated images, and RTL.
- Specification-clause-to-model-declaration traceability.
- Per-module refinement proofs composed into a larger transition-system theorem.

The Metronome project is a hardware/RTL system. Its Yosys extraction, RTL cuts, physical-design checks, and chip-specific proofs are not directly applicable to this C++ instruction-set simulator. This branch adapts verification ideas, not Metronome implementation code.

## CPU-specific adaptation

1. Keep Silica-Allah and Neuro-Silica as separate executables. They both use `namespace silica`; combining their headers or linking both engines into one process is not the comparison strategy.
2. Pin the reference checkout to the audited Silica-Allah commit `92b4e6751111a03b83080d773ed3b1b34a208d01` for reproducible differential runs.
3. Compare after every executed instruction, including PC, SP, NZCV, X0-X30, touched memory, and execution/fault outcome where the contracts overlap.
4. Keep intentionally different contracts (SVC host yield, alignment, memory sizing, unsupported-instruction halt semantics) in a separate expected-divergence suite.
5. Use independent known-answer tests so the differential oracle is not the only source of expected behavior.
6. Mutate implementation logic and require the corresponding regression test to fail. Report killed, surviving, and equivalent mutants separately.
7. Maintain an instruction coverage ledger that distinguishes implementation from test evidence and formal proof.

## First concrete defect and correction

The inspected Neuro-Silica MADD dispatch compared:

```cpp
(ins & 0xFFE08000u) == 0x9B007000u
```

The mask clears the `0x7000` bits in the comparison value, so the equality cannot be true. The old code also only handled the XZR-accumulator alias, despite being labelled MADD.

This branch changes the match value to `0x9B000000u` and computes:

```cpp
Xd = Xn * Xm + Xa;
```

with register 31 read as zero, matching the existing downstream register helper. MSUB remains unsupported and must not be inferred from this change.

Regression tests cover both a nonzero accumulator and XZR accumulator. A deterministic mutation script deliberately corrupts the decode constant and expects the native regression suite to fail.

## Commands

```sh
make native
./build/native_core_test
make mutation
make differential SILICA_ALLAH_DIR=../Silica-Allah
make test
```

The mutation target requires a C++20 compiler. Differential requires a separate Silica-Allah checkout at the pinned revision. No build, test, mutation, or differential result is claimed by this document until those commands are actually run and their complete outputs recorded.

## Proof boundary

A future Lean model may specify arithmetic, flags, branches, and memory address calculations. A proof about that model alone does not prove this C++ implementation. A formal conformance claim requires a reviewed connection between the C++ execution path and the formal semantics, in addition to the differential and native test evidence.
