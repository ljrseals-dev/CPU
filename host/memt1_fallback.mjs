/**
 * Neuro-Silica MEMT1 fallback
 * Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr.
 * SPDX-License-Identifier: MIT
 */
export const U64_MASK = (1n << 64n) - 1n;

function assertU64(value, name) {
  if (typeof value !== "bigint" || value < 0n || value > U64_MASK) {
    throw new RangeError(name + " must be unsigned 64-bit BigInt");
  }
}

function assertAddressRange(base, maxOffset, name) {
  if (!Number.isSafeInteger(base) || base < 0) {
    throw new RangeError(name + " must be a non-negative safe integer");
  }
  if (base % 8 !== 0) {
    throw new RangeError(name + " must be 8-byte aligned");
  }
  if (base > Number.MAX_SAFE_INTEGER - maxOffset) {
    throw new RangeError(name + " range exceeds the safe integer address space");
  }
}

function readU64(mem, address, name) {
  const value = mem.read64(address);
  assertU64(value, name);
  return value;
}

export function cpuMatrixUpdate(mem, matrixAddr, gamma, omega, keyAddr, valAddr) {
  // Compatibility adapter for the original callback API:
  // (read64, write64, matrix, gamma, omega, key, val).
  if (typeof mem === "function" && typeof matrixAddr === "function") {
    const read64 = mem;
    const write64 = matrixAddr;
    mem = { read64, write64 };
    [matrixAddr, gamma, omega, keyAddr, valAddr] =
      [gamma, omega, keyAddr, valAddr, arguments[6]];
  }

  if (!mem || typeof mem.read64 !== "function" ||
      typeof mem.write64 !== "function") {
    throw new TypeError("mem must provide read64 and write64");
  }

  assertAddressRange(matrixAddr, 15 * 8, "matrixAddr");
  assertAddressRange(keyAddr, 3 * 8, "keyAddr");
  assertAddressRange(valAddr, 3 * 8, "valAddr");
  assertU64(gamma, "gamma");
  assertU64(omega, "omega");

  // Snapshot every input before writing so read failures and overlapping
  // input/output regions cannot make later cells depend on earlier writes.
  const keys = Array.from({ length: 4 }, (_, i) =>
    readU64(mem, keyAddr + i * 8, "key[" + i + "]"));
  const values = Array.from({ length: 4 }, (_, j) =>
    readU64(mem, valAddr + j * 8, "val[" + j + "]"));
  const matrix = Array.from({ length: 16 }, (_, index) =>
    readU64(mem, matrixAddr + index * 8,
      "matrix[" + Math.floor(index / 4) + "," + (index % 4) + "]"));

  const outputs = new Array(16);
  for (let i = 0; i < 4; i++) {
    for (let j = 0; j < 4; j++) {
      const index = i * 4 + j;
      // Preserve the documented modulo-2^64 result AFTER the fixed-point
      // shift. Masking the product before shifting changes high-bit inputs.
      const forget = ((matrix[index] * gamma) >> 8n) & U64_MASK;
      const write = ((keys[i] * values[j] * omega) >> 8n) & U64_MASK;
      outputs[index] = (forget + write) & U64_MASK;
    }
  }

  // All reads and calculations have succeeded before the first write.
  for (let index = 0; index < 16; index++) {
    mem.write64(matrixAddr + index * 8, outputs[index]);
  }
}
