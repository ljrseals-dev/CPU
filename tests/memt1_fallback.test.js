import test from "node:test";
import assert from "node:assert/strict";
import { cpuMatrixUpdate, U64_MASK } from "../host/memt1_fallback.mjs";
import { cpuMatrixUpdate as cpuMatrixUpdateLegacy } from "../host/memt1_fallback.js";

function fixture(initial = 0n) {
  const mem = new Map();
  const read64 = (address) => {
    if (!Number.isSafeInteger(address) || address < 0 || address % 8 !== 0) {
      throw new RangeError("memory fault");
    }
    return mem.get(address) ?? initial;
  };
  const write64 = (address, value) => {
    if (!Number.isSafeInteger(address) || address < 0 || address % 8 !== 0) {
      throw new RangeError("memory fault");
    }
    mem.set(address, value & U64_MASK);
  };
  return { mem, read64, write64 };
}

test("callback API computes the documented fixed-point update", () => {
  const f = fixture();
  for (let i = 0; i < 4; i++) {
    f.write64(0x3000 + i * 8, 10n);
    f.write64(0x3100 + i * 8, 10n);
  }
  for (let i = 0; i < 16; i++) f.write64(0x2000 + i * 8, 100n);
  cpuMatrixUpdate(f.read64, f.write64, 0x2000, 128n, 128n, 0x3000, 0x3100);
  assert.equal(f.read64(0x2000), 100n);
});

test("object API preserves unsigned 64-bit wraparound", () => {
  const f = fixture();
  for (let i = 0; i < 4; i++) {
    f.write64(0x3000 + i * 8, U64_MASK);
    f.write64(0x3100 + i * 8, U64_MASK);
  }
  for (let i = 0; i < 16; i++) f.write64(0x2000 + i * 8, U64_MASK);
  cpuMatrixUpdate({ read64: f.read64, write64: f.write64 },
    0x2000, U64_MASK, U64_MASK, 0x3000, 0x3100);
  assert.ok(f.read64(0x2000) >= 0n && f.read64(0x2000) <= U64_MASK);
});

test("fallback rejects invalid unsigned inputs", () => {
  const f = fixture();
  assert.throws(() => cpuMatrixUpdate(f.read64, f.write64,
    0x2000, -1n, 1n, 0x3000, 0x3100), RangeError);
});

test("fallback rejects unaligned regions before reading or writing", () => {
  let reads = 0;
  let writes = 0;
  const mem = {
    read64: () => { reads++; return 0n; },
    write64: () => { writes++; },
  };
  assert.throws(() => cpuMatrixUpdate(mem, 0x2001, 1n, 1n, 0x3000, 0x3100),
    /8-byte aligned/);
  assert.equal(reads, 0);
  assert.equal(writes, 0);
});

test("fallback rejects address-range overflow before reading or writing", () => {
  let reads = 0;
  let writes = 0;
  const mem = {
    read64: () => { reads++; return 0n; },
    write64: () => { writes++; },
  };
  assert.throws(() => cpuMatrixUpdate(mem,
    Number.MAX_SAFE_INTEGER - 7, 1n, 1n, 0x3000, 0x3100),
  /safe integer address space/);
  assert.equal(reads, 0);
  assert.equal(writes, 0);
});

test("fallback stages all reads before the first matrix write", () => {
  let reads = 0;
  const writes = [];
  const mem = {
    read64: () => {
      reads++;
      if (reads === 10) throw new RangeError("simulated read fault");
      return 1n;
    },
    write64: (address, value) => writes.push([address, value]),
  };
  assert.throws(() => cpuMatrixUpdate(mem, 0x2000, 1n, 1n, 0x3000, 0x3100),
    /simulated read fault/);
  assert.equal(writes.length, 0);
});

test("legacy .js entry point delegates to checked MEMT1 implementation", () => {
  let reads = 0;
  let writes = 0;
  const mem = {
    read64: () => { reads++; return 0n; },
    write64: () => { writes++; },
  };
  assert.throws(() => cpuMatrixUpdateLegacy(
    mem, 0x2001, 1n, 1n, 0x3000, 0x3100), /8-byte aligned/);
  assert.equal(reads, 0);
  assert.equal(writes, 0);
});
