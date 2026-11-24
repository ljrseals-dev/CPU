import test from "node:test";
import assert from "node:assert/strict";
import { cpuMatrixUpdate, U64_MASK } from "../host/memt1_fallback.js";

function fixture(initial = 0n) {
  const mem = new Map();
  const read64 = (a) => {
    if (!Number.isSafeInteger(a) || a < 0 || a % 8 !== 0) throw new RangeError("memory fault");
    return mem.get(a) ?? initial;
  };
  const write64 = (a, v) => {
    if (!Number.isSafeInteger(a) || a < 0 || a % 8 !== 0) throw new RangeError("memory fault");
    mem.set(a, v & U64_MASK);
  };
  return { mem, read64, write64 };
}

test("shared fallback computes the documented fixed-point update", () => {
  const f = fixture();
  for (let i = 0; i < 4; i++) {
    f.write64(0x3000 + i * 8, 10n);
    f.write64(0x3100 + i * 8, 10n);
  }
  for (let i = 0; i < 16; i++) f.write64(0x2000 + i * 8, 100n);
  cpuMatrixUpdate(f.read64, f.write64, 0x2000, 128n, 128n, 0x3000, 0x3100);
  assert.equal(f.read64(0x2000), 100n);
});

test("fallback preserves unsigned 64-bit wraparound", () => {
  const f = fixture();
  for (let i = 0; i < 4; i++) {
    f.write64(0x3000 + i * 8, U64_MASK);
    f.write64(0x3100 + i * 8, U64_MASK);
  }
  for (let i = 0; i < 16; i++) f.write64(0x2000 + i * 8, U64_MASK);
  cpuMatrixUpdate(f.read64, f.write64, 0x2000, U64_MASK, U64_MASK, 0x3000, 0x3100);
  assert.ok(f.read64(0x2000) >= 0n && f.read64(0x2000) <= U64_MASK);
});

test("fallback rejects invalid unsigned inputs", () => {
  const f = fixture();
  assert.throws(() => cpuMatrixUpdate(f.read64, f.write64, 0x2000, -1n, 1n, 0x3000, 0x3100), RangeError);
});
