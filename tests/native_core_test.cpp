/**
 * Native core smoke tests | Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr. | SPDX-License-Identifier: MIT
 */
#include "../src/silica/cpu.hpp"
#include <cassert>
#include <iostream>
using namespace silica;
static void branch_test(){static Cpu c;c.reset();c.mem.store32(0x1000,0xD2800140u);c.mem.store32(0x1004,0xF100281Fu);c.mem.store32(0x1008,0x54000040u);c.mem.store32(0x100c,0xD2800041u);c.mem.store32(0x1010,0xD2800021u);c.regs.pc=0x1000;c.run(4);assert(c.regs.x[1]==1);assert(c.regs.pc==0x1014);}
static void bounds_test(){static Cpu c;c.reset();c.mem.store32(0x1000,0xF9000020u);c.regs.x[1]=Memory::SIZE;c.regs.pc=0x1000;c.run(1);assert(c.mem.fault()==FaultReason::Memory);}
static void alignment_test(){static Memory m;m.clear_fault();(void)m.load64(3);assert(m.fault()==FaultReason::UnalignedAccess);}
static void little_endian_test(){static Memory m;m.clear_fault();m.store64(0,0x0102030405060708ull);assert(m.data_[0]==0x08&&m.data_[7]==0x01);assert(m.load64(0)==0x0102030405060708ull);m.store32(8,0xA1B2C3D4u);assert(m.data_[8]==0xD4&&m.data_[11]==0xA1);assert(m.load32(8)==0xA1B2C3D4u);}
static void multiply_accumulate_test(){
 static Cpu c;c.reset();
 c.regs.x[1]=6;c.regs.x[2]=5;c.regs.x[3]=7;
 c.mem.store32(0x1000,0x9B020C20u); // MADD X0, X1, X2, X3
 c.mem.store32(0x1004,0x9B028C24u); // MSUB X4, X1, X2, X3
 c.regs.pc=0x1000;c.run(2);
 assert(c.mem.fault()==FaultReason::None);
 assert(c.regs.x[0]==37u);
 assert(c.regs.x[4]==(uint64_t(7)-uint64_t(30)));
}
int main(){branch_test();bounds_test();alignment_test();little_endian_test();multiply_accumulate_test();std::cout<<"native core smoke tests passed\n";}
