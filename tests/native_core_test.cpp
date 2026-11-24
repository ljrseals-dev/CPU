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

int main() {
  branch_test();
  bl_and_ret_register_test();
  subs_uses_sp_for_rn31_test();
  bounds_and_alignment_test();
  address_wraparound_test();
  register_offset_wraparound_test();
  little_endian_test();
  multiply_accumulate_test();
  std::cout << "native core regression tests passed\n";
}
