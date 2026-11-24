/**
 * Neuro-Silica MEMT1 fallback (ESM entry)
 * Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr.
 * SPDX-License-Identifier: MIT
 *
 * Re-exports the single canonical implementation from memt1_fallback.js
 * so unit tests and the critic harness share identical arithmetic.
 */
export { cpuMatrixUpdate, U64_MASK } from "./memt1_fallback.js";
