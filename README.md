# CPU / Neuro-Silica

**Developer / Author:** Lewis James Seals Jr. (ljrseals-dev)  
**License:** MIT

Neuro-Silica is an early AArch64 execution and MEMT1 fixed-point research prototype. It is not production-ready, does not implement complete AArch64, and makes no CPU/GPU-equivalence claim.

## Authored implementation
- host/memt1_fallback.mjs: checked JavaScript BigInt fallback.
- src/silica/cpu.hpp and cpu.cpp: bounded memory and a deliberately limited decoder.
- src/silica_wasm.cpp: minimal C ABI wrapper using static storage, without a custom allocator.
- tests/native_core_test.cpp: native branch, bounds, alignment, and MADD/MSUB regression tests.
- tests/critic_harness.test.mjs: WASM ABI and host-fallback smoke harness.
- tests/differential/probe.cpp: optional per-instruction state trace against Silica-Allah, including MADD/MSUB.\n- docs/WEBGPU_BACKEND.md: GPU arithmetic and parity contract.\n- docs/BRUTE_GENIUS_CROSS_PROJECT.md: CPU/Sogitech experiment and verification plan.

## Build
Requires a native C++20 compiler, Node.js, and Clang/LLVM wasm32 tools for WASM.
Run: make test

The GitHub authoring workflow does not execute these commands. Status remains AUTHORED — NOT TESTED until a real local or CI run records successful output. The current research branch adds MADD/MSUB coverage; the tests are authored but not yet run.

## Limitations
The decoder supports only a small instruction subset; unsupported instructions fault. It is not a general AArch64 emulator. The harness manually invokes the host fallback after SVC yield; it does not prove guest assembly computes MEMT1. No WGSL shader, WebGPU dispatch, staging/readback implementation, guest binary, GUI, or differential GPU test is included.
