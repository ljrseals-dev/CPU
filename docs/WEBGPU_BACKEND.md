# WebGPU backend contract

**Author:** Lewis James Seals Jr. (ljrseals-dev)  
**Copyright:** (c) 2026 Lewis James Seals Jr.  
**License:** MIT

## Status
Research design only. No GPU kernel is included or verified. Portable WGSL has no general native u64 scalar. A future implementation must use two u32 limbs and explicitly implement multiplication, carries, shifts, and modulo-2^64 behavior.

## Arithmetic contract
The host fallback computes each product modulo 2^64 before shifting right by eight bits, then adds both shifted terms modulo 2^64. A GPU implementation must match this order for all inputs, including overflow boundaries.

## Required verification
- Compare all 16 cells against the host fallback.
- Include randomized and adversarial overflow vectors.
- Validate staging alignment, mapping lifecycle, device-loss behavior, and readback.
- Make no CPU/GPU-equivalence claim until differential tests run on actual WebGPU.
