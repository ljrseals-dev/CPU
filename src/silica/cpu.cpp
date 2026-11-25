/**
 * Neuro-Silica core | Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr. | SPDX-License-Identifier: MIT
 */
#include "silica/cpu.hpp"

namespace silica {
namespace {
bool range_ok(uint64_t address, size_t bytes) {
  return address <= Memory::SIZE &&
         bytes <= Memory::SIZE - static_cast<size_t>(address);
}

bool add_address(uint64_t base, uint64_t offset, uint64_t& result) {
  if (offset > UINT64_MAX - base) return false;
  result = base + offset;
  return true;
}
}  // namespace

uint64_t Memory::load64(uint64_t address) {
  if (address & 7u) {
    raise_fault(FaultReason::UnalignedAccess);
    return 0;
  }
  if (!range_ok(address, 8)) {
    raise_fault(FaultReason::Memory);
    return 0;
  }
  uint64_t value = 0;
  for (unsigned i = 0; i < 8; ++i) {
    value |= uint64_t(data_[static_cast<size_t>(address) + i]) << (i * 8);
  }
  return value;
}

void Memory::store64(uint64_t address, uint64_t value) {
  if (address & 7u) {
    raise_fault(FaultReason::UnalignedAccess);
    return;
  }
  if (!range_ok(address, 8)) {
    raise_fault(FaultReason::Memory);
    return;
  }
  for (unsigned i = 0; i < 8; ++i) {
    data_[static_cast<size_t>(address) + i] =
        static_cast<uint8_t>(value >> (i * 8));
  }
}

uint32_t Memory::load32(uint64_t address) {
  if (address & 3u) {
    raise_fault(FaultReason::UnalignedAccess);
    return 0;
  }
  if (!range_ok(address, 4)) {
    raise_fault(FaultReason::Memory);
    return 0;
  }
  uint32_t value = 0;
  for (unsigned i = 0; i < 4; ++i) {
    value |= uint32_t(data_[static_cast<size_t>(address) + i]) << (i * 8);
  }
  return value;
}

void Memory::store32(uint64_t address, uint32_t value) {
  if (address & 3u) {
    raise_fault(FaultReason::UnalignedAccess);
    return;
  }
  if (!range_ok(address, 4)) {
    raise_fault(FaultReason::Memory);
    return;
  }
  for (unsigned i = 0; i < 4; ++i) {
    data_[static_cast<size_t>(address) + i] =
        static_cast<uint8_t>(value >> (i * 8));
  }
}

void Memory::wipe() {
  for (size_t i = 0; i < SIZE; ++i) data_[i] = 0;
  fault_ = FaultReason::None;
}

void Cpu::reset() {
  regs = Registers{};
  mem.wipe();
  yielded = false;
  dcache.invalidate();
}

bool Cpu::execute_one() {
  if (mem.fault() != FaultReason::None || yielded) return false;

  const uint64_t pc0 = regs.pc;
  const uint32_t ins = mem.load32(pc0);
  if (mem.fault() != FaultReason::None) return false;
  const InstructionKind kind = dcache.lookup_or_decode(pc0, ins);
  regs.pc = pc0 + 4;
  const auto fail = [this, pc0](FaultReason reason) {
    mem.raise_fault(reason);
    regs.pc = pc0;
    return false;
  };

  const auto rr = [this](unsigned r) -> uint64_t {
    return r == 31 ? 0 : regs.x[r];
  };
  const auto wr = [this](unsigned r, uint64_t value) {
    if (r != 31) regs.x[r] = value;
  };
  const auto base = [this](unsigned r) -> uint64_t {
    return r == 31 ? regs.sp : regs.x[r];
  };

  // SVC #0 is the prototype's host-yield trap when X8 carries 0xFF.
  if (kind == InstructionKind::Svc) {
    if (rr(8) == 0xFFu) {
      yielded = true;
      return false;
    }
    return true;
  }

  // RET Xn (including the conventional RET X30).
  if (kind == InstructionKind::Ret) {
    const unsigned rn = (ins >> 5) & 31u;
    regs.pc = rr(rn);
    return true;
  }

  // MOVZ Xd, #imm{, LSL #shift}.
  if (kind == InstructionKind::MovZ) {
    const unsigned rd = ins & 31u;
    const unsigned shift = ((ins >> 21) & 3u) * 16u;
    const uint64_t immediate = (ins >> 5) & 0xFFFFu;
    wr(rd, immediate << shift);
    return true;
  }

  // SUBS Xd, Xn|SP, #imm{, LSL #12}; Rd=31 discards into XZR.
  if (kind == InstructionKind::SubsImm) {
    const unsigned rn = (ins >> 5) & 31u;
    const uint64_t immediate =
        uint64_t((ins >> 10) & 0xFFFu) << (((ins >> 22) & 1u) * 12u);
    const uint64_t a = base(rn);  // Rn=31 denotes SP for this encoding.
    const uint64_t result = a - immediate;
    const bool n = (result >> 63) != 0;
    const bool z = result == 0;
    const bool c = a >= immediate;
    const bool v = (((a ^ immediate) & (a ^ result)) >> 63) != 0;
    regs.nzcv = (n ? 8u : 0u) | (z ? 4u : 0u) |
                (c ? 2u : 0u) | (v ? 1u : 0u);
    wr(ins & 31u, result);  // Rd=31 is XZR; other destinations receive the result.
    return true;
  }

  // B.cond.
  if (kind == InstructionKind::BCond) {
    const unsigned condition = ins & 15u;
    const int32_t offset = static_cast<int32_t>(ins << 8) >> 13;
    const bool n = (regs.nzcv & 8u) != 0;
    const bool z = (regs.nzcv & 4u) != 0;
    const bool c = (regs.nzcv & 2u) != 0;
    const bool v = (regs.nzcv & 1u) != 0;
    bool take = false;
    switch (condition) {
      case 0: take = z; break;                 // EQ
      case 1: take = !z; break;                // NE
      case 2: take = c; break;                 // CS/HS
      case 3: take = !c; break;                // CC/LO
      case 4: take = n; break;                 // MI
      case 5: take = !n; break;                // PL
      case 6: take = v; break;                 // VS
      case 7: take = !v; break;                // VC
      case 8: take = c && !z; break;           // HI
      case 9: take = !c || z; break;           // LS
      case 10: take = n == v; break;           // GE
      case 11: take = n != v; break;           // LT
      case 12: take = !z && (n == v); break;   // GT
      case 13: take = z || (n != v); break;    // LE
      case 14: take = true; break;             // AL
      default: break;                          // NV/reserved in this subset
    }
    if (take) regs.pc = pc0 + static_cast<int64_t>(offset) * 4;
    return true;
  }

  // B and BL share the imm26 layout; BL additionally writes X30.
  if (kind == InstructionKind::Branch) {
    const int32_t offset = static_cast<int32_t>(ins << 6) >> 6;
    if ((ins & 0x80000000u) != 0) regs.x[30] = pc0 + 4;
    regs.pc = pc0 + static_cast<int64_t>(offset) * 4;
    return true;
  }

  // LDR/STR X, [Xn|SP, #imm12 * 8].
  if (kind == InstructionKind::LdrStrUnsigned) {
    const unsigned rt = ins & 31u;
    const unsigned rn = (ins >> 5) & 31u;
    const uint64_t offset = uint64_t((ins >> 10) & 0xFFFu) * 8u;
    uint64_t address = 0;
    if (!add_address(base(rn), offset, address)) {
      return fail(FaultReason::Memory);
    }
    if ((ins & 0xFFC00000u) == 0xF9400000u) {
      const uint64_t value = mem.load64(address);
      if (mem.fault() != FaultReason::None) {
        regs.pc = pc0;
        return false;
      }
      wr(rt, value);
    } else {
      mem.store64(address, rr(rt));
      if (mem.fault() != FaultReason::None) {
        regs.pc = pc0;
        return false;
      }
    }
    return true;
  }

  // LDR/STR X, [Xn|SP, Xm, LSL #3].
  if (kind == InstructionKind::LdrStrRegister) {
    const unsigned rt = ins & 31u;
    const unsigned rn = (ins >> 5) & 31u;
    const unsigned rm = (ins >> 16) & 31u;
    const uint64_t index = rr(rm);
    if (index > (UINT64_MAX >> 3)) {
      return fail(FaultReason::Memory);
    }
    uint64_t address = 0;
    if (!add_address(base(rn), index << 3, address)) {
      return fail(FaultReason::Memory);
    }
    if ((ins & 0xFFE0FC00u) == 0xF8607800u) {
      const uint64_t value = mem.load64(address);
      if (mem.fault() != FaultReason::None) {
        regs.pc = pc0;
        return false;
      }
      wr(rt, value);
    } else {
      mem.store64(address, rr(rt));
      if (mem.fault() != FaultReason::None) {
        regs.pc = pc0;
        return false;
      }
    }
    return true;
  }

  // ADD (shifted register), 32-bit and 64-bit forms.
  // Extended-register ADD is intentionally not implemented (bit 21 must be 0).
  if (kind == InstructionKind::AddShifted) {
    const bool is_64 = (ins >> 31) != 0;
    const unsigned rd = ins & 31u;
    const unsigned rn = (ins >> 5) & 31u;
    const unsigned rm = (ins >> 16) & 31u;
    const unsigned shift = (ins >> 22) & 3u;
    const unsigned amount = (ins >> 10) & 63u;
    const unsigned width = is_64 ? 64u : 32u;
    if (shift == 3u || amount >= width) {
      return fail(FaultReason::UnsupportedOpcode);
    }

    uint64_t operand = rr(rm);
    if (!is_64) operand = static_cast<uint32_t>(operand);
    if (amount != 0) {
      if (shift == 0u) {
        operand = is_64 ? (operand << amount)
                        : static_cast<uint32_t>(operand << amount);
      } else if (shift == 1u) {
        operand = is_64 ? (operand >> amount)
                        : static_cast<uint32_t>(operand >> amount);
      } else if (is_64) {
        // Portable arithmetic shift: avoid implementation-defined signed shift.
        operand = (operand >> amount) |
                 ((operand & (uint64_t(1) << 63))
                      ? (UINT64_MAX << (64u - amount)) : 0u);
      } else {
        const uint32_t word = static_cast<uint32_t>(operand);
        operand = (word >> amount) |
                 ((word & (uint32_t(1) << 31))
                      ? (UINT32_MAX << (32u - amount)) : 0u);
      }
    }

    const uint64_t lhs = is_64 ? rr(rn)
                              : static_cast<uint32_t>(rr(rn));
    const uint64_t result = is_64 ? lhs + operand
                                  : static_cast<uint32_t>(lhs + operand);
    wr(rd, result);
    return true;
  }

  // A64 32-bit and 64-bit MADD/MSUB; W results zero-extend.
  const uint32_t multiply_opcode = ins & 0x7FE08000u;
  if (kind == InstructionKind::MaddMsub) {
    const bool is_64 = (ins >> 31) != 0;
    const unsigned rd = ins & 31u;
    const unsigned rn = (ins >> 5) & 31u;
    const unsigned ra = (ins >> 10) & 31u;
    const unsigned rm = (ins >> 16) & 31u;
    const uint64_t lhs = is_64 ? rr(rn) : static_cast<uint32_t>(rr(rn));
    const uint64_t rhs = is_64 ? rr(rm) : static_cast<uint32_t>(rr(rm));
    const uint64_t accumulator = is_64 ? rr(ra) : static_cast<uint32_t>(rr(ra));
    const uint64_t product = lhs * rhs;
    const uint64_t result = multiply_opcode == 0x1B008000u
                                ? accumulator - product
                                : accumulator + product;
    wr(rd, is_64 ? result : static_cast<uint32_t>(result));
    return true;
  }

  return fail(FaultReason::UnsupportedOpcode);
}

void Cpu::run(int instruction_count) {
  for (int i = 0; i < instruction_count; ++i) {
    if (!execute_one()) break;
  }
}
}  // namespace silica
