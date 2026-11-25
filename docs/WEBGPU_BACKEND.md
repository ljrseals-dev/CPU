# WebGPU backend contract

**Author:** Lewis James Seals Jr. (ljrseals-dev)  
**Copyright:** (c) 2026 Lewis James Seals Jr.  
**License:** MIT

## Status

Research design only. No GPU kernel is included or verified. Portable WGSL has no general native u64 scalar. A future implementation must use two u32 limbs and explicitly implement multiplication, carries, shifts, and modulo-2^64 behavior.

## Arithmetic contract

For each matrix cell, the host fallback evaluates the products at full BigInt precision, shifts each product right by eight bits, and only then truncates each shifted term to unsigned 64-bit. It adds the two shifted terms and truncates the sum to unsigned 64-bit:

`forget = ((matrix[i,j] * gamma) >> 8) mod 2^64`

`write = ((key[i] * val[j] * omega) >> 8) mod 2^64`

`matrix'[i,j] = (forget + write) mod 2^64`

**Do not truncate either product to 64 bits before shifting.** That produces different results for high-bit operands. The current `host/memt1_fallback.mjs` implementation, its critic-harness expected value, and the original fallback formula use full-product shift followed by truncation.

A future GPU implementation must preserve the high product bits needed by the right shift, then return the low 64 bits of each shifted term. It must match the host fallback for all inputs, including overflow boundaries.

## Required verification

- Compare all 16 cells against the host fallback.
- Include randomized and adversarial overflow vectors that distinguish shift-then-truncate from truncate-then-shift.
- Validate staging alignment, mapping lifecycle, device-loss behavior, and readback.
- Make no CPU/GPU-equivalence claim until differential tests run on actual WebGPU.
