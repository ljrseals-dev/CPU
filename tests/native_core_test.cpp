/**
 * Native core smoke tests | Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr. | SPDX-License-Identifier: MIT
 */
#include "../src/silica/cpu.hpp"
#include <cassert>
#include <iostream>
using namespace silica;
static void branch_test(){static Cpu c;c.reset();c.mem.store32(0x1000,0xD2800140u);c.mem.store32(0x1004,0xF100281Fu);c.mem.store32(0x1008,0x54000040u);c.mem.store32(0x100c,0xD2800041u);c.mem.store32(0x1010,0xD2800021u);c.regs.pc=0x1000;c.run(4);assert(c.regs.x[1]==1);assert(c.regs.pc==0x1014);}
static void bounds_test(){static Cpu c;c.reset();c.mem.store32(0x1000,0xF9000020u);c.regs.x[1]=Memory::SIZE-4;c.regs.pc=0x1000;c.run(1);assert(c.mem.fault()==FaultReason::Memory);}
static void alignment_test(){static Memory m;m.clear_fault();(void)m.load64(3);assert(m.fault()==FaultReason::UnalignedAccess);}
int main(){branch_test();bounds_test();alignment_test();std::cout<<"native core smoke tests passed\n";}
