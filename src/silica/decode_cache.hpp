/**
 * Neuro-Silica decode cache | Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr. | SPDX-License-Identifier: MIT
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace silica {

enum class InstructionKind : uint8_t {
  Invalid,
  Svc,
  Ret,
  MovZ,
  SubsImm,
  BCond,
  Branch,
  LdrStrUnsigned,
  LdrStrRegister,
  AddShifted,
  MaddMsub
};

constexpr InstructionKind classify_instruction(uint32_t ins) noexcept {
  if (ins == 0xD4000001u) return InstructionKind::Svc;
  if ((ins & 0xFFFFFC1Fu) == 0xD65F0000u) return InstructionKind::Ret;
  if ((ins & 0xFF800000u) == 0xD2800000u) return InstructionKind::MovZ;
  if ((ins & 0xFF800000u) == 0xF1000000u) return InstructionKind::SubsImm;
  if ((ins & 0xFF000010u) == 0x54000000u) return InstructionKind::BCond;
  if ((ins & 0x7C000000u) == 0x14000000u) return InstructionKind::Branch;
  if ((ins & 0xFFC00000u) == 0xF9400000u ||
      (ins & 0xFFC00000u) == 0xF9000000u)
    return InstructionKind::LdrStrUnsigned;
  if ((ins & 0xFFE0FC00u) == 0xF8607800u ||
      (ins & 0xFFE0FC00u) == 0xF8207800u)
    return InstructionKind::LdrStrRegister;
  if ((ins & 0x1F200000u) == 0x0B000000u &&
      (ins & 0x60000000u) == 0)
    return InstructionKind::AddShifted;
  const uint32_t multiply_opcode = ins & 0x7FE08000u;
  if (multiply_opcode == 0x1B000000u ||
      multiply_opcode == 0x1B008000u)
    return InstructionKind::MaddMsub;
  return InstructionKind::Invalid;
}

constexpr size_t DECODE_CACHE_SETS = 4096;
constexpr size_t DECODE_CACHE_MASK = DECODE_CACHE_SETS - 1;
static_assert(DECODE_CACHE_SETS != 0 &&
              (DECODE_CACHE_SETS & (DECODE_CACHE_SETS - 1)) == 0,
              "decode cache set count must be a power of two");

struct DecodeCacheEntry {
  uint64_t pc = UINT64_MAX;
  uint32_t word = 0;
  InstructionKind kind = InstructionKind::Invalid;
  bool valid = false;
};

class DecodeCache {
 public:
  InstructionKind lookup_or_decode(uint64_t pc, uint32_t word) noexcept {
    const size_t index = static_cast<size_t>((pc >> 2) & DECODE_CACHE_MASK);
    auto& entry = entries_[index];
    if (entry.valid && entry.pc == pc && entry.word == word)
      return entry.kind;

    const InstructionKind kind = classify_instruction(word);
    entry.pc = pc;
    entry.word = word;
    entry.kind = kind;
    entry.valid = true;
    return kind;
  }

  void invalidate() noexcept {
    for (auto& entry : entries_) entry.valid = false;
  }

 private:
  DecodeCacheEntry entries_[DECODE_CACHE_SETS]{};
};

}  // namespace silica
