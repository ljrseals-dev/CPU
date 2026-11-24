// Shared MEMT1 fixed-point fallback. Arithmetic is explicitly modulo 2^64.
const U64_MASK = (1n << 64n) - 1n;

function assertU64(value, name) {
  if (typeof value !== "bigint" || value < 0n || value > U64_MASK) {
    throw new RangeError(name + " must be an unsigned 64-bit BigInt");
  }
}

function assertAddress(value, name) {
  if (!Number.isSafeInteger(value) || value < 0) {
    throw new RangeError(name + " must be a non-negative safe integer address");
  }
}

/**
 * Apply M[i,j] = ((M[i,j] * gamma) >> 8) +
 *                ((key[i] * val[j] * omega) >> 8), modulo 2^64.
 *
 * read64/write64 are supplied by the WASM host. They must perform bounds and
 * alignment checks; this function validates arithmetic inputs and address math.
 */
export function cpuMatrixUpdate(read64, write64, matrixAddr, gamma, omega, keyAddr, valAddr) {
  if (typeof read64 !== "function" || typeof write64 !== "function") {
    throw new TypeError("read64 and write64 must be functions");
  }
  assertAddress(matrixAddr, "matrixAddr");
  assertAddress(keyAddr, "keyAddr");
  assertAddress(valAddr, "valAddr");
  assertU64(gamma, "gamma");
  assertU64(omega, "omega");

  for (let i = 0; i < 4; i++) {
    const k = read64(keyAddr + i * 8);
    assertU64(k, "key[" + i + "]");
    for (let j = 0; j < 4; j++) {
      const v = read64(valAddr + j * 8);
      assertU64(v, "val[" + j + "]");
      const addr = matrixAddr + (i * 4 + j) * 8;
      const m = read64(addr);
      assertU64(m, "matrix[" + i + "," + j + "]");
      const forget = ((m * gamma) >> 8n) & U64_MASK;
      const write = ((k * v * omega) >> 8n) & U64_MASK;
      write64(addr, (forget + write) & U64_MASK);
    }
  }
}

export { U64_MASK };
