# Silica-Allah Upstream Compatibility Audit

**Author:** Lewis James Seals Jr. (ljrseals-dev)  
**Copyright:** Copyright (c) 2026 Lewis James Seals Jr.  
**SPDX-License-Identifier:** MIT  
**Status:** SOURCE-INSPECTED — NOT BUILD-VERIFIED  
**Audit date:** 2026-11-24

## 1. Scope and immutable baselines

This audit compares the public source trees of:

- Upstream simulator: [ljrseals-dev/Silica-Allah](https://github.com/ljrseals-dev/Silica-Allah), branch `main`, inspected commit `92b4e6751111a03b83080d773ed3b1b34a208d01`.
- Downstream research prototype: [ljrseals-dev/CPU](https://github.com/ljrseals-dev/CPU), branch `main`, inspected commit `2e5f8c6ba7bbee566049450a966c720bb6c55040`.

No files were changed in Silica-Allah. This audit is documentation-only in the downstream repository. It does not claim that the two implementations are API-compatible, instruction-equivalent, or independently validated.

The upstream README reports 164 unit-test checks. The test source was inspected, but the test executable was not run in this audit. The downstream README explicitly labels its implementation AUTHORED — NOT TESTED; that status remains unchanged.

## 2. Executive result

**Compatibility: NOT ESTABLISHED.** Neuro-Silica's current `src/silica/` core is a separate, deliberately small interpreter. It is not presently a port or drop-in consumer of the Silica-Allah header-only simulator.

The two projects have materially different public APIs, architectural state, memory models, instruction coverage, fault behavior, and build contracts. The downstream code must not claim upstream conformance merely because both projects use the `silica` namespace or decode AArch64 instructions.

## 3. Source-level comparison

| Area | Silica-Allah upstream | Neuro-Silica downstream | Finding |
|---|---|---|---|
| Language/build | C++20; `std::span`, `std::endian`, `std::bit`; GCC/Clang | C++20; separate native and wasm32 targets | Both currently require C++20. No C++17 compatibility claim is supported. |
| Public headers | `include/silica/*.hpp`; header-only core | `src/silica/cpu.hpp` plus `cpu.cpp` | Different include paths and class contracts. |
| CPU API | `Cpu::load_program`, `step`, `run(uint64_t)`; `HaltReason` | `Cpu::reset`, `run(int)`; public `yielded` | Not source-compatible. |
| Register state | X0-X30/XZR, separate SP, V0-V31, PC, PSTATE, fixed system-register slots | X0-X30, SP, PC, packed NZCV | SIMD/vector and system-register state absent downstream. |
| Memory | Runtime-sized `FlatMemory`, default 1 MiB, byte operations via templated read/write, sticky fault | Fixed 16 MiB `Memory`, 32/64-bit operations, alignment faults | Different size, API, and alignment semantics. |
| Decode/execute | Hand-written decode tree and grouped semantics; broad documented integer/control/load-store subset | Hand-written opcode checks inside `execute_one`; small selected subset | No decoder reuse and no demonstrated semantic parity. |
| Unsupported instruction | Decodes to Invalid; CPU halts with `HaltReason::Invalid` | Raises `UnsupportedOpcode` memory fault | Different externally visible failure contract. |
| SVC | Not implemented; documented as Invalid / no exception model | One SVC encoding is a host-yield convention | Downstream SVC behavior is an application-specific extension, not upstream behavior. |
| Endianness | Compile-time little-endian host requirement | memcpy-based access without equivalent compile-time guard | Downstream host assumption should be explicit or byte-order-independent. |
| Build/test | `make test`, `make san`, `make bench` | `make test` invokes native, wasm, and Node smoke checks | Separate build pipelines; neither test suite substitutes for the other. |

## 4. Instruction coverage gap

Silica-Allah documents support for, among other things:

- ADD/SUB immediate and shifted-register forms, including flag-setting forms;
- ADC/SBC, logical immediate and shifted-register operations;
- MOVZ/MOVN/MOVK, ADR/ADRP;
- variable shifts, MADD/MSUB, UDIV/SDIV, CSEL-family operations;
- direct, conditional, compare-and-branch, test-and-branch, register branches and link/return;
- GPR byte/halfword/word/doubleword loads and stores, sign-extending forms, pre/post/unscaled forms, and pairs;
- a limited MRS/MSR system-register set, NOP/hints, and barrier no-ops.

Silica-Allah explicitly documents omissions including extended-register add/sub, CCMP/CCMN, bitfield/extract aliases, selected pair and register-offset memory forms, LSE atomics, pointer authentication, SVC/ERET/exception modeling, and SIMD/FP/SVE/SME.

The downstream `execute_one` source currently handles a much smaller set: one SVC yield convention, RET, MOVZ, SUBS-immediate, B.cond, B, unsigned-offset 64-bit LDR/STR, selected register-offset 64-bit LDR/STR, ADD shifted-register matching, and a restricted MADD pattern. It does not implement the upstream decoder or semantics. Treat every other encoding as unsupported until implemented and tested.

## 5. Specific compatibility and correctness risks

### P0 — Do not advertise upstream equivalence

There is no shared decoder, semantic adapter, or differential test suite. The downstream critic harness manually invokes the JavaScript fallback after an SVC yield. Its own final message correctly limits the claim to ABI and host-fallback smoke coverage; it does not demonstrate guest computation or GPU parity.

### P1 — Register and system-state mismatch

Upstream has a structured PSTATE and fixed system-register slots, plus vector registers. Downstream stores NZCV as a packed integer and has no corresponding vector/system-register model. A direct state copy or ABI bridge would silently lose architectural state.

### P1 — Memory behavior mismatch

Upstream supports configurable memory size and uses unaligned accesses as documented behavior for its current functional model. Downstream imposes 4/8-byte alignment checks and has a fixed 16 MiB store. The same guest memory instruction can therefore succeed upstream and fault downstream.

### P1 — Fault and halt contract mismatch

Upstream distinguishes ZeroWord, Brk, Invalid, and MemFault halt reasons. Downstream exposes a sticky `FaultReason` and a separate `yielded` flag. An integration layer needs an explicit mapping; it must not collapse unsupported opcodes, memory faults, and host yields into one status.

### P1 — SVC is not an upstream feature

The downstream special case for `0xD4000001` is a private host-bridge convention. Upstream explicitly lists SVC and the exception model as unimplemented. Do not route arbitrary SVC instructions through the MEMT1 host callback or describe this as architectural exception handling.

### P2 — Host-endian assumption

Upstream rejects non-little-endian hosts at compile time. Downstream uses `memcpy` for scalar memory operations but has no corresponding guard or byte-order conversion. Make the intended target constraint explicit before claiming portable native behavior.

### P2 — Resource and WASM layout assumptions

The downstream `Cpu` embeds a 16 MiB memory array and the WASM target reserves a fixed 32 MiB linear memory. The C ABI returns a pointer into that static storage. This is a separate memory-layout contract from upstream's dynamically sized `FlatMemory`; any reuse must account for WASM memory growth, pointer validity, and the exact linear-memory layout.

## 6. Upstream memory review note

The upstream `FlatMemory::read<T>` and `write<T>` bounds checks use `size_ - sizeof(T)`. The constructor guarantees a minimum allocation of 16 bytes, but the templates do not constrain `T` to be no larger than the allocation. A caller instantiating a larger `T` can make the subtraction underflow. Current simulator call sites use architectural scalar widths, but the public template contract is wider than the safe bound-check implementation. Consider guarding `sizeof(T) > size_` before subtracting and constraining supported types.

This is a source-review finding, not a demonstrated exploit or a failing upstream test.

## 7. Test interpretation

- Upstream README advertises 164 checks and provides `make test`, `make san`, and `make bench`.
- Upstream `tests/test_main.cpp` covers known encodings, immediate and register data processing, NZCV behavior, branches, GPR loads/stores, and the limited system-register set.
- Downstream native tests cover a branch smoke path, one out-of-bounds store, and one alignment fault.
- Downstream Node tests cover the host fallback's fixed-point arithmetic, unsigned wraparound, and invalid unsigned inputs.
- Downstream critic harness checks a WASM ABI/yield path and then invokes the host fallback manually.

These test sets have different purposes. Passing one is not evidence that the other implementation conforms. No differential test currently feeds identical instruction streams and initial states to both engines and compares PC, GPRs, SP, NZCV/PSTATE, memory, and halt/fault outcomes.

## 8. Required integration sequence

1. Preserve Silica-Allah `main` as the upstream reference; do not modify it as part of downstream integration.
2. Decide whether Neuro-Silica should embed/adapt the upstream core or remain an intentionally independent minimal interpreter. Do not maintain two undocumented semantic authorities.
3. If adapting, define a narrow adapter for CPU state, memory, stepping, and fault mapping. Keep the MEMT1 SVC host-yield extension outside upstream architectural semantics.
4. Add a differential harness using the upstream core as the reference for the common supported subset. Compare after every step, not only final output.
5. Add instruction-family coverage for every instruction emitted by the actual guest workload; unsupported instructions must fail closed with a precise diagnostic.
6. Run upstream `make test` and `make san`, downstream native tests, Node tests, and WASM build/runtime tests in a real toolchain environment. Record compiler versions and complete output.
7. Only after those results exist should the downstream README make a compatibility or validation claim.

## 9. Verification record

- GitHub repository metadata and recursive source trees inspected for both repositories.
- Upstream branch list inspected: `main` is the only branch at audit time.
- Upstream source inspected at `92b4e6751111a03b83080d773ed3b1b34a208d01`.
- Downstream source inspected at `2e5f8c6ba7bbee566049450a966c720bb6c55040`.
- Local clone/build attempt could not proceed because the execution environment could not resolve `github.com`; no compiler or test result is claimed.
- No source code, tests, or build files were modified by this audit.
- Audit result: **DOCUMENTED — NOT BUILD-VERIFIED; COMPATIBILITY NOT ESTABLISHED**.
