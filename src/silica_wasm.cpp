/**
 * Neuro-Silica WASM C ABI | Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr. | SPDX-License-Identifier: MIT
 */
#include "silica/cpu.hpp"
using namespace silica;
// Static storage: no custom allocator and no second 16 MiB heap allocation.
static Cpu cpu;static bool initialized=false;
extern "C" {
void init_cpu(uint64_t entry){cpu.reset();cpu.regs.pc=entry;initialized=true;}
void step(int n){if(initialized&&n>0)cpu.run(n);}
void set_reg(int i,uint64_t v){if(!initialized)return;if(i>=0&&i<31)cpu.regs.x[i]=v;else if(i==31)cpu.regs.sp=v;else if(i==32)cpu.regs.pc=v;}
uint64_t get_reg(int i){if(!initialized)return 0;if(i>=0&&i<31)return cpu.regs.x[i];if(i==31)return cpu.regs.sp;if(i==32)return cpu.regs.pc;return 0;}
int get_fault(){return initialized?static_cast<int>(cpu.mem.fault()):0;}
uint8_t* memory_base(){return initialized?cpu.mem.data_:nullptr;}
int get_yielded(){return initialized&&cpu.yielded?1:0;}
void clear_yielded(){if(initialized)cpu.yielded=false;}
void reset_cpu(){if(initialized)cpu.reset();}
}
