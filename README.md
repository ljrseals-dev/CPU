# CPU / Neuro-Silica

This repository is being brought from an empty scaffold toward the v3.1 CPU +
WebGPU research prototype. It is **not production-ready** and does not yet
claim a verified AArch64 implementation or CPU/GPU equivalence.

## Current implementation slice

- `host/memt1_fallback.js` defines the shared MEMT1 fixed-point CPU fallback.
- Arithmetic is unsigned modulo 2^64, with the shift and addition order made
  explicit.
- `tests/memt1_fallback.test.js` exercises the shared implementation with
  Node's built-in test runner.

## Precision and verification status

The intended WebGPU design represents each 64-bit value as two 32-bit limbs.
Portable WGSL has no native `u64`; the shader must implement 32-bit partial
products and carries without converting values to floating point. No GPU shader
is included or claimed verified yet. CPU/GPU byte-equivalence has **not** been
verified for any range.

## Run the current tests

```sh
node --test tests/memt1_fallback.test.js
```

## Remaining v3.1 work

AArch64 decoder and NZCV semantics, explicit fault reasons, RET and halt
hypercall semantics, checked memory access, WASM ABI, limb-arithmetic WGSL,
WebGPU staging readback/resource lifetime, guest assembly and reproducible
binary generation, GUI error handling, differential GPU tests, and CI remain
to be implemented and verified.
