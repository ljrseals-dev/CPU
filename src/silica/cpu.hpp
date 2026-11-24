/**
 * Neuro-Silica core | Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr. | SPDX-License-Identifier: MIT
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
namespace silica {
enum class FaultReason : uint8_t {
  None = 0,
  Memory = 1,
  UnsupportedOpcode = 2,
  UnalignedAccess = 3
};

struct Registers {
  uint64_t x[31]{};
  uint64_t sp = 0;
  uint64_t pc = 0;
  uint32_t nzcv = 0;
};

// Guest RAM lives in a fixed-size buffer owned by Memory.
// For the WASM target a freestanding memset in silica_wasm.cpp satisfies BSS zero-init.
class Memory {
public:
  static constexpr size_t SIZE = 16u * 1024u * 1024u;
  uint8_t data_[SIZE]{};
  FaultReason fault_ = FaultReason::None;

  FaultReason fault() const { return fault_; }
  void clear_fault() { fault_ = FaultReason::None; }
  void raise_fault(FaultReason f) {
    if (fault_ == FaultReason::None) fault_ = f;
  }

  uint64_t load64(uint64_t);
  void store64(uint64_t, uint64_t);
  uint32_t load32(uint64_t);
  void store32(uint64_t, uint32_t);
  void wipe();
};

class Cpu {
public:
  Registers regs{};
  Memory mem{};
  bool yielded = false;
  void reset();
  void run(int);

private:
  bool execute_one();
};
} // namespace silica
