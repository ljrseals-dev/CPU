# C++20 Silica Differential Harness

**Author:** Lewis James Seals Jr. (ljrseals-dev)  
**Copyright:** Copyright (c) 2026 Lewis James Seals Jr.  
**SPDX-License-Identifier:** MIT  
**Status:** DRAFT — AUTHORED, NOT BUILD-VERIFIED  
**Upstream baseline:** `ljrseals-dev/Silica-Allah` `main` at `92b4e6751111a03b83080d773ed3b1b34a208d01`  
**Downstream baseline:** `ljrseals-dev/CPU` `main` at `9754ec91203994e6350e38c29409753276952ba8`

## Purpose

Compare Neuro-Silica with the Silica-Allah reference on a deliberately narrow set of A64 instructions implemented by both. Each engine is compiled into a separate executable because both expose classes in `namespace silica`; linking or including both implementations in one translation unit would create conflicting declarations.

The probe emits a normalized architectural-state trace after every executed instruction. The Make target compares the traces byte-for-byte. This detects the first step at which PC, SP, NZCV, any X0-X30 register, or the selected data-memory word differs.

## Layout

- `tests/differential/probe.cpp`: shared probe source; compile once with `SILICA_REFERENCE` and once with `SILICA_NEURO`.
- `Makefile`: opt-in `differential` target.
- `build/differential/reference.trace`: upstream trace.
- `build/differential/neuro.trace`: downstream trace.

## Run

Place both repositories side-by-side, or provide the upstream checkout explicitly:

```sh
make differential SILICA_ALLAH_DIR=../Silica-Allah
```

The default is `SILICA_ALLAH_DIR=../Silica-Allah`. The target uses the configured `CXX` compiler and C++20. It is intentionally not part of the ordinary `make test` target because the upstream checkout is an explicit prerequisite.

A successful `diff` means only that the two executables emitted identical traces for this fixed program and these initial conditions. It does not establish general ISA conformance, complete instruction coverage, memory equivalence, exception equivalence, or production readiness.

## Initial common-subset program

The ten probe steps cover:

1. `MOVZ X0, #10`
2. `MOVZ X1, #5`
3. `ADD X0, X0, X1` (unshifted register form)
4. `SUBS XZR, X0, #15` (NZCV update)
5. `B.EQ +8` (taken conditional branch)
6. The branch skips a decoy `MOVZ X2, #99`
7. `MOVZ X2, #7`
8. `STR X2, [X3]`
9. `LDR X4, [X3]`
10. `ADD X5, X2, X4`

The test seeds X3 with address `0x8000`, uses code base `0x1000`, and starts with a known 64-bit data word. The probe compares the selected memory word after every step.

## Deliberate exclusions

- SVC / MEMT1 host-yield behavior: downstream-only convention; upstream does not implement A64 exception entry.
- MADD/MSUB: excluded until both encodings and downstream dispatch are independently validated.
- Unaligned and out-of-range accesses: memory contracts differ by design.
- SIMD/FP, system registers, pair accesses, and other instruction families: not in this initial intersection.
- Halt, Invalid, and fault translation: different engine contracts; no false equivalence is implied.

## Exit behavior

- Exit 0 from a probe: all requested instructions executed without that engine halting or faulting.
- Nonzero probe exit: initialization or execution failed; diagnostic identifies engine and step.
- Nonzero `diff`: the traces differ. Preserve both trace files and investigate the first differing line; do not update expected output to mask a mismatch.

## Next work

1. Run the target with both checkouts at the pinned revisions.
2. Add independent known-answer checks for each instruction encoding before expanding the intersection.
3. Add bounded deterministic input vectors (edge values, carry/overflow boundaries, branch taken/not-taken).
4. Add memory boundary cases only after specifying a common memory contract.
5. Add a separate expected-divergence suite for intentional contract differences; never mix those with the equivalence suite.
6. Integrate the target into CI only after the upstream checkout is pinned reproducibly (for example, as a read-only submodule or explicit CI checkout at the audited SHA).

## Verification record

The source and build target are authored in this branch. They have not been compiled or executed in this environment. No passing result is claimed.
