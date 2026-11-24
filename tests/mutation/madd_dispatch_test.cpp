// Focused MADD dispatch regression used by the mutation harness.
// SPDX-License-Identifier: MIT
#include "../../src/silica/cpu.hpp"

#include <cstdint>
#include <iostream>

int main() {
  static silica::Cpu cpu;
  cpu.reset();
  cpu.regs.x[1] = 6;
  cpu.regs.x[2] = 7;
  cpu.regs.x[3] = 5;
  cpu.mem.store32(0x1000, 0x9B020C20u);  // MADD X0, X1, X2, X3
  cpu.mem.store32(0x1004, 0x9B027C24u);  // MADD X4, X1, X2, XZR
  cpu.regs.pc = 0x1000;

  cpu.run(1);
  if (cpu.mem.fault() != silica::FaultReason::None ||
      cpu.regs.x[0] != 47 || cpu.regs.pc != 0x1004) {
    std::cerr << "MADD accumulator case failed\n";
    return 1;
  }

  cpu.run(1);
  if (cpu.mem.fault() != silica::FaultReason::None ||
      cpu.regs.x[4] != 42 || cpu.regs.pc != 0x1008) {
    std::cerr << "MADD XZR accumulator case failed\n";
    return 1;
  }

  std::cout << "MADD regression passed\n";
  return 0;
}
