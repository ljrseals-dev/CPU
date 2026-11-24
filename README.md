# CPU / Neuro-Silica

**Developer / Author:** Lewis James Seals Jr. (ljrseals-dev)  
**License:** MIT

Neuro-Silica is an early AArch64 execution and MEMT1 fixed-point research prototype. It is not production-ready, does not implement complete AArch64, and makes no CPU/GPU-equivalence claim.

## Authored implementation
- host/memt1_fallback.mjs: checked JavaScript BigInt fallback.
- src/silica/cpu.hpp and cpu.cpp: bounded memory and a deliberately limited decoder.
- src/silica_wasm.cpp: minimal C ABI wrapper using static storage, without a custom allocator.
- tests/native_core_test.cpp: native branch-condition, BL/RET, SP, bounds, alignment, address-overflow, endian, and MADD/MSUB regressions.
- tests/memt1_fallback.test.js: fixed-point, range, alignment, and read-failure regression tests.
- tests/critic_harness.test.mjs: WASM ABI and host-fallback smoke harness.
- tests/differential/probe.cpp: per-instruction state trace against pinned Silica-Allah, including MADD/MSUB.
- docs/WEBGPU_BACKEND.md: GPU arithmetic and parity contract.
- docs/BRUTE_GENIUS_CROSS_PROJECT.md: CPU/Sogitech experiment and verification plan.
- docs/BUGFIX_AUDIT_2026-11-24.md: current focused source audit and verification boundaries.

## Build
Requires a native C++20 compiler, Node.js, and Clang/LLVM wasm32 tools for WASM.

```sh
make clean
make test
make san
```

The test target builds and runs native regressions, builds the WASM module, runs the Node fixed-point tests and WASM ABI/host-fallback harness, then verifies that a deliberately broken BL link-register implementation is rejected by the native suite. The separate `make san` target runs the native regression suite with AddressSanitizer and UndefinedBehaviorSanitizer.

Optional pinned-reference state trace (requires a Silica-Allah checkout):
`make differential SILICA_ALLAH_DIR=../Silica-Allah`

The pre-existing baseline passed the full native/Node/WASM suite under GCC and Clang in GitHub Actions and passed the pinned Silica-Allah lockstep trace for its declared common instruction subset. The current bug-fix branch has passed the GCC/Clang native, WASM, Node, mutation, and pinned-reference differential jobs. Sanitizer results are tracked separately for this branch.

## Limitations
The decoder supports only a small instruction subset; unsupported instructions fault. It is not a general AArch64 emulator. The harness manually invokes the host fallback after SVC yield; it does not prove guest assembly computes MEMT1. No WGSL shader, WebGPU dispatch, staging/readback implementation, guest binary, GUI, or differential GPU test is included.
