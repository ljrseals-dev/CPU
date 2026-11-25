# CPU / Neuro-Silica

**Developer / Author:** Lewis James Seals Jr. (ljrseals-dev)  
**License:** MIT

Neuro-Silica is an early AArch64 functional-simulation and MEMT1 fixed-point research prototype. It is not production-ready, does not implement complete AArch64, and makes no CPU/GPU-equivalence claim.

## Implemented architecture

- Fixed 16 MiB guest RAM, little-endian 32-bit instruction fetch and 32-/64-bit data access.
- Register state: X0-X30, SP, PC, and NZCV.
- Fault categories: memory, unsupported opcode, and unaligned access.
- The decoder is intentionally limited to the instruction subsets below.

## Decoder support matrix

| Instruction family | Implemented subset | Important limits |
|---|---|---|
| MOVZ | 64-bit immediate form | W-form MOVZ and MOVK are not implemented |
| ADD (shifted register) | W/X forms with LSL, LSR, ASR | ROR and extended-register forms are unsupported; no flag-setting ADDS |
| SUBS (immediate) | 64-bit immediate form; Rn=31 reads SP | Other SUB/SUBS forms are unsupported |
| B / BL | Immediate branch; BL writes X30 | BR, BLR, and conditional compare/test branches are unsupported |
| B.cond | Conditions EQ through AL (0–14) | Reserved condition 15 faults as unsupported |
| RET | RET Xn, including RET X30 | Other branch-register forms are unsupported |
| LDR / STR | 64-bit unsigned immediate offset scaled by 8 | No W-form, byte/halfword, pre/post-index, pair, literal, or SIMD loads/stores |
| LDR / STR | Selected 64-bit register-offset form with LSL #3 | Other extension and scaling modes are unsupported |
| MADD / MSUB | W/X integer forms with accumulator register | No long multiply, signed/unsigned high-half multiply, or SIMD multiply |
| SVC | Exact SVC #0 host-yield convention when X8 is 0xFF | Prototype host callback convention, not general AArch64 exception handling |

Unsupported encodings raise an unsupported-opcode fault. Data memory accesses enforce alignment and bounds; effective-address arithmetic is checked before access. Failed instructions restore PC to the faulting instruction. The SVC host-yield path intentionally leaves PC at the following instruction so the host can resume.

## Repository map

- host/memt1_fallback.mjs — checked JavaScript BigInt MEMT1 fallback with input snapshots before writes.
- src/silica/cpu.hpp and src/silica/cpu.cpp — fixed-memory CPU state and limited instruction decoder.
- src/silica_wasm.cpp — minimal WASM C ABI wrapper using static storage.
- tests/native_core_test.cpp — branch, condition, memory, fault-PC, shifted ADD, and W/X MADD/MSUB regressions.
- tests/memt1_fallback.test.js — fixed-point, range, alignment, and read-failure regressions.
- tests/critic_harness.test.mjs — WASM ABI and host-fallback smoke harness.
- tests/mutation/ — BL link-register and MADD dispatch mutation checks.
- tests/differential/probe.cpp — per-instruction state trace against a pinned Silica-Allah reference.
- docs/METRONOME_VERIFICATION_ADAPTATION.md — research adaptation and verification approach.
- docs/VERIFICATION_COVERAGE.md — instruction evidence ledger.
- docs/BUGFIX_AUDIT_2026-11-24.md — source-reviewed bug-fix audit and verification boundaries.
- docs/WEBGPU_BACKEND.md — GPU arithmetic and parity contract; no implemented WebGPU backend is claimed.
- docs/BRUTE_GENIUS_CROSS_PROJECT.md — CPU/Sogitech experiment and verification plan.

## Build and verification

Requirements: C++20 compiler, Node.js 22, Clang/LLVM WebAssembly compiler and linker.

Run the maintained test targets:

    make clean
    make test
    make san

The test target builds native code and WASM, runs the native regressions, Node MEMT1 tests, the WASM ABI/host-fallback harness, and both BL and MADD mutation checks.

Run the pinned-reference differential trace when a separate Silica-Allah checkout is available:

    make differential SILICA_ALLAH_DIR=../Silica-Allah

CI exercises GCC and Clang native builds, WASM linking, Node tests, ASan/UBSan, mutation checks, and the pinned Silica-Allah differential trace. GitHub Actions run #32 passed on the MADD mutation port, and run #35 passed on the documentation-preservation pull request's tested source snapshot. These results cover the tested code and cases only; they do not establish complete AArch64 conformance or formal equivalence.

## Explicit limitations

- This is not a complete AArch64 emulator. The decoder implements only the tabled subset.
- There is no ELF loader, operating-system model, MMU, cache hierarchy, SMP, SIMD/FP unit, interrupt controller, or general exception model.
- The SVC host-yield behavior is a private prototype convention. The harness manually invokes the host fallback; it does not prove guest assembly computes MEMT1.
- There is no implemented WGSL shader, WebGPU dispatch, staging/readback path, GUI, guest binary, or differential GPU test.
- The fixed-memory WASM wrapper is a minimal C ABI surface, not a complete runtime or production sandbox.
- Differential agreement applies only to the exact shared instructions and states exercised. Formal proof of a standalone model would not, by itself, prove this C++ implementation.

Neuro-Silica remains research software. Do not use it as a production processor emulator or infer unsupported instruction behavior.
