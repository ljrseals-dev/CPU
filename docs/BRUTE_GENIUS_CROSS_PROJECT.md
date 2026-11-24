# Brute Genius cross-project verification contract

**Author:** Lewis James Seals Jr. (ljrseals-dev)  
**SPDX-License-Identifier:** MIT  
**Status:** RESEARCH PLAN — NOT EXECUTED

## Purpose

Define a reproducible experiment boundary between Neuro-Silica, Sogitech Codec,
and the externally maintained brentius/metronome repository. The projects are
not to be merged and no implementation is copied across repository boundaries
without an explicit compatibility review.

## Repository roles

- **Neuro-Silica (this repository):** instruction semantics, fixed-width
  arithmetic, memory behavior, native/WASM ABI, and differential state traces.
- **Sogitech Codec:** DSP analysis/resynthesis, parameter quantization, neural
  parameter prediction, and measured audio quality/latency.
- **brentius/metronome:** read-only research reference. Its branch names and
  source are not evidence that its tests or methods have been run here. Its
  owner retains control of its repository.

## Immediate correctness gate

The existing multiply-accumulate decoder condition compared a masked value
against `0x9B007000`, although the mask `0xFFE08000` clears the accumulator
register field. The condition therefore could not match a valid MADD encoding.
It also restricted the accumulator to register 31 and omitted MSUB.

The research branch corrects the decode to distinguish MADD (`0x9B000000`)
and MSUB (`0x9B008000`) using the masked opcode, then reads Ra from bits
[14:10]. Regression vectors cover a non-zero accumulator. This source change
is authored but not yet build-verified.

## Shared experiment protocol

Every experiment must record:

1. Repository, branch, commit SHA, compiler/runtime version, and host target.
2. Candidate parameters and deterministic seed.
3. Reference implementation and input fixture hash.
4. Correctness result before any performance score is considered.
5. Warm-up policy, repetition count, median and spread.
6. Peak memory, latency, throughput, and failure diagnostics.
7. Raw machine-readable result plus a human-readable summary.

No candidate is promoted on a single timing result. Correctness failures are
hard rejects, not weighted penalties.

## CPU experiment families

- MADD/MSUB and other integer arithmetic: instruction encoding, operand
  corner cases, XZR behavior, unsigned wraparound, and reference state trace.
- Memory: bounds, alignment, aliasing, and endian assumptions.
- Native versus WASM: identical fixtures, explicit ABI checks, and no
  CPU/GPU parity claim without an implemented GPU backend and readback test.
- Differential execution: compare state after each instruction for the
  declared common instruction subset; stop on the first divergence.

## Sogitech experiment families

- Quantization: 4/8/12-bit and uniform/perceptual layouts.
- Analysis: pitch tracker, envelope estimator, and filter mapping.
- Residual policies: none, high-pass residual, harmonic subtraction.
- Controller: none, MLP 64x128, MLP 64x256; neural candidates require real,
  trained checkpoints and source-grouped data splits.
- Audio quality: spectral and multi-resolution STFT errors, parameter error,
  residual-energy error, peak level, and measured latency.

The Python offline renderer is a reference renderer, not a bit-exact
replacement for the Cmajor processor. Audio claims require separate Cmajor
compilation and host validation.

## Cross-project hypotheses (not conclusions)

1. Use Sogitech's fixed-point/quantized parameter fixtures to exercise CPU
   integer arithmetic and overflow boundaries.
2. Use CPU's deterministic state-trace discipline to specify Sogitech's
   quantizer/dequantizer round-trip invariants.
3. Compare scalar reference arithmetic against future optimized CPU paths
   only after a working reference implementation and test vectors exist.
4. Borrow Metronome verification ideas only after reviewing the exact source
   and confirming license, semantics, and suitability. Do not import RTL,
   physical-design assumptions, or tool-specific scripts into the CPU/DSP
   codebases merely because they share the word "optimization."

## Acceptance gates

- [ ] MADD/MSUB regression tests compile and pass.
- [ ] Existing CPU native, Node, and WASM tests pass.
- [ ] Differential trace tests run against the pinned Silica-Allah reference.
- [ ] Sogitech candidate enumeration tests pass and candidate order is stable.
- [ ] Every evaluated candidate uses the same source-grouped fixtures and seed.
- [ ] Cmajor compile and host validation pass before real-time audio claims.
- [ ] Results include raw data and are reproducible from documented commands.

## Current evidence status

- Source inspection identified a decoder predicate defect in MADD/MSUB.
- Regression test source has been added on this research branch.
- No compiler, test runner, Cmajor toolchain, Brute Genius worker, or external
  Metronome job has been executed as part of this authoring step.
- No measured performance improvement is claimed.
