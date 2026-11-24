/**
 * Neuro-Silica core | Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr. | SPDX-License-Identifier: MIT
 */
#include "silica/cpu.hpp"

namespace silica {

static bool range_ok(uint64_t a, size_t n) {
  return a <= Memory::SIZE && n <= Memory::SIZE - static_cast<size_t>(a);
}

uint64_t Memory::load64(uint64_t a) {
  if (a & 7u) {
    raise_fault(FaultReason::UnalignedAccess);
    return 0;
  }
  if (!range_ok(a, 8)) {
    raise_fault(FaultReason::Memory);
    return 0;
  }
  uint64_t v = 0;
  for (unsigned i = 0; i < 8; i++)
    v |= uint64_t(data_[static_cast<size_t>(a) + i]) << (i * 8);
  return v;
}

void Memory::store64(uint64_t a, uint64_t v) {
  if (a & 7u) {
    raise_fault(FaultReason::UnalignedAccess);
    return;
  }
  if (!range_ok(a, 8)) {
    raise_fault(FaultReason::Memory);
    return;
  }
  for (unsigned i = 0; i < 8; i++)
    data_[static_cast<size_t>(a) + i] = static_cast<uint8_t>(v >> (i * 8));
}

uint32_t Memory::load32(uint64_t a) {
  if (a & 3u) {
    raise_fault(FaultReason::UnalignedAccess);
    return 0;
  }
  if (!range_ok(a, 4)) {
    raise_fault(FaultReason::Memory);
    return 0;
  }
  uint32_t v = 0;
  for (unsigned i = 0; i < 4; i++)
    v |= uint32_t(data_[static_cast<size_t>(a) + i]) << (i * 8);
  return v;
}

void Memory::store32(uint64_t a, uint32_t v) {
  if (a & 3u) {
    raise_fault(FaultReason::UnalignedAccess);
    return;
  }
  if (!range_ok(a, 4)) {
    raise_fault(FaultReason::Memory);
    return;
  }
  for (unsigned i = 0; i < 4; i++)
    data_[static_cast<size_t>(a) + i] = static_cast<uint8_t>(v >> (i * 8));
}

void Memory::wipe() {
  for (size_t i = 0; i < SIZE; i++)
    data_[i] = 0;
  fault_ = FaultReason::None;
}

void Cpu::reset() {
  regs = Registers{};
  mem.wipe();
  yielded = false;
}

bool Cpu::execute_one() {
  if (mem.fault() != FaultReason::None || yielded)
    return false;

  const uint64_t pc0 = regs.pc;
  const uint32_t ins = mem.load32(pc0);
  if (mem.fault() != FaultReason::None)
    return false;
  regs.pc = pc0 + 4;

  auto rr = [&](unsigned r) -> uint64_t {
    return r == 31 ? 0 : regs.x[r];
  };
  auto wr = [&](unsigned r, uint64_t v) {
    if (r != 31)
      regs.x[r] = v;
  };
  auto base = [&](unsigned r) -> uint64_t {
    return r == 31 ? regs.sp : regs.x[r];
  };

  // SVC #0 with X8 == 0xFF → host yield convention
  if (ins == 0xD4000001u) {
    if (rr(8) == 0xFFu) {
      yielded = true;
      return false;
    }
    return true;
  }

  // RET (BR X30)
  if (ins == 0xD65F03C0u) {
    regs.pc = rr(30);
    return true;
  }

  // MOVZ Xd, #imm16, LSL #shift
  if ((ins & 0xFF800000u) == 0xD2800000u) {
    unsigned rd = ins & 31u;
    unsigned sh = ((ins >> 21) & 3u) * 16u;
    uint64_t imm = (ins >> 5) & 0xffffu;
    wr(rd, imm << sh);
    return true;
  }

  // SUBS (immediate): write Rd and set NZCV (CMP when Rd == XZR)
  if ((ins & 0xFF800000u) == 0xF1000000u) {
    unsigned rd = ins & 31u;
    unsigned rn = (ins >> 5) & 31u;
    uint64_t b = ((ins >> 10) & 0xfffu) << (((ins >> 22) & 1u) * 12u);
    uint64_t a = rr(rn);
    uint64_t res = a - b;
    bool n = res >> 63;
    bool z = res == 0;
    bool c = a >= b;
    bool v = (((a ^ b) & (a ^ res)) >> 63) != 0;
    regs.nzcv = (n ? 8u : 0u) | (z ? 4u : 0u) | (c ? 2u : 0u) | (v ? 1u : 0u);
    wr(rd, res);
    return true;
  }

  // B.cond
  if ((ins & 0xFF000010u) == 0x54000000u) {
    unsigned c = ins & 15u;
    int32_t off = static_cast<int32_t>(ins << 8) >> 13;
    bool n = regs.nzcv & 8u;
    bool z = regs.nzcv & 4u;
    bool cy = regs.nzcv & 2u;
    bool v = regs.nzcv & 1u;
    bool t = false;
    switch (c) {
    case 0: t = z; break;
    case 1: t = !z; break;
    case 2: t = cy; break;
    case 3: t = !cy; break;
    case 4: t = n; break;
    case 5: t = !n; break;
    case 6: t = v; break;
    case 7: t = !v; break;
    case 8: t = cy && !z; break;
    case 9: t = !cy || z; break;
    case 10: t = n == v; break;
    case 11: t = n != v; break;
    case 12: t = !z && (n == v); break;
    case 13: t = z || (n != v); break;
    case 14: t = true; break;
    default: break;
    }
    if (t)
      regs.pc = pc0 + static_cast<int64_t>(off) * 4;
    return true;
  }

  // B (unconditional)
  if ((ins & 0x7C000000u) == 0x14000000u) {
    int32_t off = static_cast<int32_t>(ins << 6) >> 6;
    regs.pc = pc0 + static_cast<int64_t>(off) * 4;
    return true;
  }

  // LDR/STR unsigned offset (imm12 * 8), 64-bit
  if ((ins & 0xFFC00000u) == 0xF9400000u ||
      (ins & 0xFFC00000u) == 0xF9000000u) {
    unsigned rt = ins & 31u;
    unsigned rn = (ins >> 5) & 31u;
    uint64_t a = base(rn) + (((ins >> 10) & 0xfffu) * 8u);
    if ((ins & 0xFFC00000u) == 0xF9400000u) {
      uint64_t x = mem.load64(a);
      if (mem.fault() != FaultReason::None)
        return false;
      wr(rt, x);
    } else {
      mem.store64(a, rr(rt));
      if (mem.fault() != FaultReason::None)
        return false;
    }
    return true;
  }

  // LDR/STR register offset (Rm << 3), 64-bit
  if ((ins & 0xFFE0FC00u) == 0xF8607800u ||
      (ins & 0xFFE0FC00u) == 0xF8207800u) {
    unsigned rt = ins & 31u;
    unsigned rn = (ins >> 5) & 31u;
    unsigned rm = (ins >> 16) & 31u;
    uint64_t a = base(rn) + (rr(rm) << 3);
    if ((ins & 0xFFE0FC00u) == 0xF8607800u) {
      uint64_t x = mem.load64(a);
      if (mem.fault() != FaultReason::None)
        return false;
      wr(rt, x);
    } else {
      mem.store64(a, rr(rt));
      if (mem.fault() != FaultReason::None)
        return false;
    }
    return true;
  }

  // ADD (shifted register), LSL #0 form only
  if ((ins & 0xFFE0FC00u) == 0x8B000000u) {
    unsigned rd = ins & 31u;
    unsigned rn = (ins >> 5) & 31u;
    unsigned rm = (ins >> 16) & 31u;
    wr(rd, rr(rn) + rr(rm));
    return true;
  }

  // MADD / MSUB: Ra is a real operand
  const uint32_t multiply_opcode = ins & 0xFFE08000u;
  if (multiply_opcode == 0x9B000000u || multiply_opcode == 0x9B008000u) {
    unsigned rd = ins & 31u;
    unsigned rn = (ins >> 5) & 31u;
    unsigned ra = (ins >> 10) & 31u;
    unsigned rm = (ins >> 16) & 31u;
    const uint64_t product = rr(rn) * rr(rm);
    wr(rd, multiply_opcode == 0x9B000000u ? rr(ra) + product
                                          : rr(ra) - product);
    return true;
  }

  mem.raise_fault(FaultReason::UnsupportedOpcode);
  return false;
}

void Cpu::run(int n) {
  for (int i = 0; i < n; i++)
    if (!execute_one())
      break;
}

} // namespace silica
