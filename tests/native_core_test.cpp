/**
 * Native core regression tests | Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr. | SPDX-License-Identifier: MIT
 */
#include "../src/silica/cpu.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

using namespace silica;

static void branch_test() {
  static Cpu cpu;
  cpu.reset();
  cpu.mem.store32(0x1000, 0xD2800140u);
  cpu.mem.store32(0x1004, 0xF100281Fu);
  cpu.mem.store32(0x1008, 0x54000040u);
  cpu.mem.store32(0x100C, 0xD2800041u);
  cpu.mem.store32(0x1010, 0xD2800021u);
  cpu.regs.pc = 0x1000;
  cpu.run(4);
  assert(cpu.regs.x[1] == 1);
  assert(cpu.regs.pc == 0x1014);
}


static bool expected_condition(unsigned condition, unsigned nzcv) {
  const bool n = (nzcv & 8u) != 0;
  const bool z = (nzcv & 4u) != 0;
  const bool c = (nzcv & 2u) != 0;
  const bool v = (nzcv & 1u) != 0;
  switch (condition) {
    case 0: return z;
    case 1: return !z;
    case 2: return c;
    case 3: return !c;
    case 4: return n;
    case 5: return !n;
    case 6: return v;
    case 7: return !v;
    case 8: return c && !z;
    case 9: return !c || z;
    case 10: return n == v;
    case 11: return n != v;
    case 12: return !z && (n == v);
    case 13: return z || (n != v);
    case 14: return true;
    default: return false;
  }
}

static void all_branch_conditions_test() {
  static Cpu cpu;
  for (unsigned condition = 0; condition < 15; ++condition) {
    for (unsigned flags = 0; flags < 16; ++flags) {
      cpu.reset();
      cpu.regs.nzcv = flags;
      cpu.mem.store32(0x1000, 0x54000040u | condition);  // B.cond +8
      cpu.regs.pc = 0x1000;
      cpu.run(1);
      const uint64_t expected_pc =
          expected_condition(condition, flags) ? 0x1008u : 0x1004u;
      assert(cpu.regs.pc == expected_pc);
      assert(cpu.mem.fault() == FaultReason::None);
    }
  }
}

static void bl_and_ret_register_test() {
  static Cpu cpu;
  cpu.reset();
  cpu.mem.store32(0x1000, 0x94000002u);  // BL +8
  cpu.mem.store32(0x1008, 0xD65F0060u);  // RET X3
  cpu.regs.x[3] = 0x1010;
  cpu.regs.pc = 0x1000;
  cpu.run(1);
  assert(cpu.regs.pc == 0x1008);
  assert(cpu.regs.x[30] == 0x1004);
  cpu.run(1);
  assert(cpu.regs.pc == 0x1010);
}

static void subs_uses_sp_for_rn31_test() {
  static Cpu cpu;
  cpu.reset();
  cpu.regs.sp = 5;
  cpu.mem.store32(0x1000, 0xF10007FFu);  // SUBS XZR, SP, #1
  cpu.regs.pc = 0x1000;
  cpu.run(1);
  assert(cpu.regs.nzcv == 0x2u);  // 5 - 1: positive, nonzero, no overflow, carry.
  assert(cpu.regs.sp == 5);       // SUBS does not write SP when Rd is XZR.
}

static void bounds_and_alignment_test() {
  static Cpu cpu;
  cpu.reset();
  cpu.mem.store32(0x1000, 0xF9000020u);  // STR X0, [X1]
  cpu.regs.x[1] = Memory::SIZE;
  cpu.regs.pc = 0x1000;
  cpu.run(1);
  assert(cpu.mem.fault() == FaultReason::Memory);

  static Memory memory;
  memory.clear_fault();
  (void)memory.load64(3);
  assert(memory.fault() == FaultReason::UnalignedAccess);
}

static void address_wraparound_test() {
  static Cpu cpu;
  cpu.reset();
  cpu.regs.x[0] = 0xA5A5A5A5A5A5A5A5ull;
  cpu.regs.x[1] = std::numeric_limits<uint64_t>::max() - 7u;
  cpu.mem.store32(0x1000, 0xF9000420u);  // STR X0, [X1, #8]
  cpu.regs.pc = 0x1000;
  cpu.run(1);
  assert(cpu.mem.fault() == FaultReason::Memory);
  assert(cpu.mem.load64(0) == 0);  // Wrapped write must not corrupt address zero.
}

static void register_offset_wraparound_test() {
  static Cpu cpu;
  cpu.reset();
  cpu.regs.x[0] = 0x1122334455667788ull;
  cpu.regs.x[1] = std::numeric_limits<uint64_t>::max() - 7u;
  cpu.regs.x[2] = 1;
  cpu.mem.store32(0x1000, 0xF8227820u);  // STR X0, [X1, X2, LSL #3]
  cpu.regs.pc = 0x1000;
  cpu.run(1);
  assert(cpu.mem.fault() == FaultReason::Memory);
  assert(cpu.mem.load64(0) == 0);
}

static void little_endian_test() {
  static Memory memory;
  memory.clear_fault();
  memory.store64(0, 0x0102030405060708ull);
  assert(memory.data_[0] == 0x08 && memory.data_[7] == 0x01);
  assert(memory.load64(0) == 0x0102030405060708ull);
  memory.store32(8, 0xA1B2C3D4u);
  assert(memory.data_[8] == 0xD4 && memory.data_[11] == 0xA1);
  assert(memory.load32(8) == 0xA1B2C3D4u);
}


static constexpr uint32_t add_shifted(unsigned is_64, unsigned shift,
                                      unsigned amount, unsigned rm,
                                      unsigned rn, unsigned rd) {
  return (is_64 ? 0x8B000000u : 0x0B000000u) |
         (shift << 22) | (rm << 16) | (amount << 10) |
         (rn << 5) | rd;
}

static void add_shifted_register_test() {
  static Cpu cpu;
  cpu.reset();
  cpu.regs.x[1] = 5;
  cpu.regs.x[2] = 7;
  cpu.regs.x[3] = 0x8000000000000000ull;
  cpu.mem.store32(0x1000, add_shifted(1, 0, 3, 2, 1, 0));  // ADD X0,X1,X2,LSL #3
  cpu.mem.store32(0x1004, add_shifted(1, 1, 1, 2, 1, 4));  // ADD X4,X1,X2,LSR #1
  cpu.mem.store32(0x1008, add_shifted(1, 2, 1, 3, 1, 5));  // ADD X5,X1,X3,ASR #1
  cpu.mem.store32(0x100C, add_shifted(0, 0, 2, 2, 1, 6));  // ADD W6,W1,W2,LSL #2
  cpu.mem.store32(0x1010, add_shifted(0, 1, 1, 2, 1, 7));  // ADD W7,W1,W2,LSR #1
  cpu.regs.pc = 0x1000;
  cpu.run(5);
  assert(cpu.mem.fault() == FaultReason::None);
  assert(cpu.regs.x[0] == 61u);
  assert(cpu.regs.x[4] == 8u);
  assert(cpu.regs.x[5] == 0xC000000000000005ull);
  assert(cpu.regs.x[6] == 33u);
  assert(cpu.regs.x[7] == 8u);

  cpu.reset();
  cpu.mem.store32(0x1000, add_shifted(1, 3, 0, 2, 1, 0));  // reserved ROR shift
  cpu.regs.pc = 0x1000;
  cpu.run(1);
  assert(cpu.mem.fault() == FaultReason::UnsupportedOpcode);

  cpu.reset();
  cpu.mem.store32(0x1000, add_shifted(0, 0, 32, 2, 1, 0));  // invalid W shift amount
  cpu.regs.pc = 0x1000;
  cpu.run(1);
  assert(cpu.mem.fault() == FaultReason::UnsupportedOpcode);
}

static void multiply_accumulate_test() {
  static Cpu cpu;
  cpu.reset();
  cpu.regs.x[1] = 6;
  cpu.regs.x[2] = 5;
  cpu.regs.x[3] = 7;
  cpu.mem.store32(0x1000, 0x9B020C20u);  // MADD X0, X1, X2, X3
  cpu.mem.store32(0x1004, 0x9B028C24u);  // MSUB X4, X1, X2, X3
  cpu.regs.pc = 0x1000;
  cpu.run(2);
  assert(cpu.mem.fault() == FaultReason::None);
  assert(cpu.regs.x[0] == 37u);
  assert(cpu.regs.x[4] == (uint64_t(7) - uint64_t(30)));
}


static void multiply_accumulate_32bit_test() {
  static Cpu cpu;
  cpu.reset();
  cpu.regs.x[1] = 0xDEADBEEF00000006ull;
  cpu.regs.x[2] = 0xFFFFFFFF00000005ull;
  cpu.regs.x[3] = 0xAAAAAAAA00000007ull;
  cpu.mem.store32(0x1000, 0x1B020C20u);  // MADD W0, W1, W2, W3
  cpu.mem.store32(0x1004, 0x1B028C24u);  // MSUB W4, W1, W2, W3
  cpu.regs.pc = 0x1000;
  cpu.run(2);
  assert(cpu.mem.fault() == FaultReason::None);
  assert(cpu.regs.x[0] == 37u);
  assert(cpu.regs.x[4] == 0xFFFFFFE9u);
}

static void fault_pc_rollback_test() {
  static Cpu cpu;

  cpu.reset();
  cpu.mem.store32(0x1000, 0xFFFFFFFFu);  // unsupported instruction
  cpu.regs.pc = 0x1000;
  cpu.run(1);
  assert(cpu.mem.fault() == FaultReason::UnsupportedOpcode);
  assert(cpu.regs.pc == 0x1000);

  cpu.reset();
  cpu.regs.x[1] = Memory::SIZE;
  cpu.mem.store32(0x1000, 0xF9400020u);  // LDR X0, [X1] -> bounds fault
  cpu.regs.pc = 0x1000;
  cpu.run(1);
  assert(cpu.mem.fault() == FaultReason::Memory);
  assert(cpu.regs.pc == 0x1000);

  cpu.reset();
  cpu.regs.x[1] = 3;
  cpu.mem.store32(0x1000, 0xF9400020u);  // LDR X0, [X1] -> alignment fault
  cpu.regs.pc = 0x1000;
  cpu.run(1);
  assert(cpu.mem.fault() == FaultReason::UnalignedAccess);
  assert(cpu.regs.pc == 0x1000);

  cpu.reset();
  cpu.regs.x[8] = 0xFF;
  cpu.mem.store32(0x1000, 0xD4000001u);  // prototype SVC #0 host-yield
  cpu.regs.pc = 0x1000;
  cpu.run(1);
  assert(cpu.yielded);
  assert(cpu.mem.fault() == FaultReason::None);
  assert(cpu.regs.pc == 0x1004);  // yield resumes after the SVC instruction
}

int main() {
  branch_test();
  all_branch_conditions_test();
  bl_and_ret_register_test();
  subs_uses_sp_for_rn31_test();
  bounds_and_alignment_test();
  address_wraparound_test();
  register_offset_wraparound_test();
  little_endian_test();
  multiply_accumulate_test();
  multiply_accumulate_32bit_test();
  add_shifted_register_test();
  fault_pc_rollback_test();
  std::cout << "native core regression tests passed\n";
}
