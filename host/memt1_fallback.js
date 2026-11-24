// Shared MEMT1 fixed-point fallback. Arithmetic is explicitly modulo 2^64.
// Canonical formula (matches unit tests and critic harness):
//   M[i,j] = ((M[i,j] * gamma) >> 8) + ((key[i] * val[j] * omega) >> 8)  (mod 2^64)
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
 * Apply the fixed-point matrix update in place via host-supplied accessors.
 *
 * Supports both call shapes for compatibility:
 *   cpuMatrixUpdate(read64, write64, matrixAddr, gamma, omega, keyAddr, valAddr)
 *   cpuMatrixUpdate({read64, write64}, matrixAddr, gamma, omega, keyAddr, valAddr)
 */
export function cpuMatrixUpdate(memOrRead, writeOrMatrix, matrixAddr, gamma, omega, keyAddr, valAddr) {
  let read64, write64, mAddr, g, o, kAddr, vAddr;

  if (typeof memOrRead === "function" && typeof writeOrMatrix === "function") {
    read64 = memOrRead;
    write64 = writeOrMatrix;
    mAddr = matrixAddr;
    g = gamma;
    o = omega;
    kAddr = keyAddr;
    vAddr = valAddr;
  } else if (memOrRead && typeof memOrRead.read64 === "function" && typeof memOrRead.write64 === "function") {
    read64 = memOrRead.read64.bind(memOrRead);
    write64 = memOrRead.write64.bind(memOrRead);
    mAddr = writeOrMatrix;
    g = matrixAddr;
    o = gamma;
    kAddr = omega;
    vAddr = keyAddr;
  } else {
    throw new TypeError("mem must provide read64/write64 functions or be a {read64,write64} object");
  }

  assertAddress(mAddr, "matrixAddr");
  assertAddress(kAddr, "keyAddr");
  assertAddress(vAddr, "valAddr");
  assertU64(g, "gamma");
  assertU64(o, "omega");

  for (let i = 0; i < 4; i++) {
    const k = read64(kAddr + i * 8);
    assertU64(k, "key[" + i + "]");
    for (let j = 0; j < 4; j++) {
      const v = read64(vAddr + j * 8);
      assertU64(v, "val[" + j + "]");
      const addr = mAddr + (i * 4 + j) * 8;
      const m = read64(addr);
      assertU64(m, "matrix[" + i + "," + j + "]");
      // Full-product shift then mask (canonical unsigned 64-bit fixed-point)
      const forget = ((m * g) >> 8n) & U64_MASK;
      const write = ((k * v * o) >> 8n) & U64_MASK;
      write64(addr, (forget + write) & U64_MASK);
    }
  }
}

export { U64_MASK };
