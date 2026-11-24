/**
 * WASM critic harness | Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr. | SPDX-License-Identifier: MIT
 */
import fs from "node:fs";
import assert from "node:assert/strict";
import { cpuMatrixUpdate, U64_MASK } from "../host/memt1_fallback.mjs";

const bytes = fs.readFileSync(new URL("../build/silica-core.wasm", import.meta.url));

// Current silica_wasm.cpp ships a freestanding memset; instantiate with empty imports.
// Keep a fallback path for older builds that still import env.memset.
let importObject = {};
let instance;
try {
  ({ instance } = await WebAssembly.instantiate(bytes, importObject));
} catch (e) {
  if (String(e).includes("env") || String(e).includes("memset")) {
    const memFill = { buf: null };
    importObject = {
      env: {
        memset(ptr, val, len) {
          if (!memFill.buf) return ptr;
          memFill.buf.fill(val & 0xff, Number(ptr), Number(ptr) + Number(len));
          return ptr;
        },
      },
    };
    ({ instance } = await WebAssembly.instantiate(bytes, importObject));
    memFill.buf = new Uint8Array(instance.exports.memory.buffer);
  } else {
    throw e;
  }
}

const e = instance.exports;
assert.ok(e.memory && e.memory_base && e.init_cpu, "required WASM ABI exports missing");

// init_cpu resets guest memory; populate program and data afterward.
e.init_cpu(0x1000n);
const view = new DataView(e.memory.buffer);
const base = Number(e.memory_base());
const matrix = 0x2000;
const keys = 0x3000;
const vals = 0x3100;
const m = 1n << 63n;
const k = 10n;
const v = 10n;
const gamma = 255n;
const omega = 128n;
const expected =
  ((((m * gamma) >> 8n) & U64_MASK) +
    (((k * v * omega) >> 8n) & U64_MASK)) &
  U64_MASK;

for (let i = 0; i < 16; i++) view.setBigUint64(base + matrix + i * 8, m, true);
for (let i = 0; i < 4; i++) {
  view.setBigUint64(base + keys + i * 8, k, true);
  view.setBigUint64(base + vals + i * 8, v, true);
}
view.setUint32(base + 0x1000, 0xD4000001, true); // SVC #0
view.setUint32(base + 0x1004, 0xD65F03C0, true); // RET
e.set_reg(8, 0xffn);
e.step(1);

assert.equal(e.get_yielded(), 1);
assert.equal(e.get_reg(32), 0x1004n);

const mem = {
  read64: (address) => view.getBigUint64(base + address, true),
  write64: (address, value) => view.setBigUint64(base + address, value, true),
};
cpuMatrixUpdate(mem, matrix, gamma, omega, keys, vals);
for (let i = 0; i < 16; i++) {
  assert.equal(view.getBigUint64(base + matrix + i * 8, true), expected);
}

e.clear_yielded();
e.step(1);
assert.equal(e.get_reg(32), 0n);
console.log(
  "critic harness passed: ABI and host fallback smoke only; no guest-compute or GPU parity claim"
);
