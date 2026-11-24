/**
 * Differential state probe for Neuro-Silica and Silica-Allah.
 * Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr. | SPDX-License-Identifier: MIT
 *
 * Build this translation unit twice, in separate binaries:
 *   -D SILICA_REFERENCE with Silica-Allah/include first on the include path
 *   -D SILICA_NEURO with Neuro-Silica/src first on the include path
 *
 * Do not link both engines into one process: both publish namespace silica.
 */
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#if defined(SILICA_REFERENCE) && defined(SILICA_NEURO)
#error "Build exactly one engine per probe binary"
#elif defined(SILICA_REFERENCE)
#include "silica/cpu.hpp"
#elif defined(SILICA_NEURO)
#include "silica/cpu.hpp"
#else
#error "Define SILICA_REFERENCE or SILICA_NEURO"
#endif

namespace {
constexpr uint64_t kCodeBase = 0x1000;
constexpr uint64_t kDataBase = 0x8000;

// MOVZ x0,#10; MOVZ x1,#5; ADD x0,x0,x1; SUBS xzr,x0,#15;
// B.EQ +8 (skip the decoy); MOVZ x2,#99; MOVZ x2,#7;
// STR x2,[x3]; LDR x4,[x3]; ADD x5,x2,x4; MADD x7,x1,x2,x4; MSUB x8,x1,x2,x4.
constexpr std::array<uint32_t, 12> kProgram{{
    0xD2800140u,
    0xD28000A1u,
    0x8B010000u,
    0xF1003C1Fu,
    0x54000040u,
    0xD2800C62u,
    0xD28000E2u,
    0xF9000062u,
    0xF9400064u,
    0x8B040045u,
    0x9B021027u,
    0x9B029028u,
}};
constexpr std::size_t kExecutedSteps = kProgram.size() - 1;  // B.EQ skips one word.

void emit_u64(uint64_t value) {
  std::cout << std::hex << std::setfill('0') << std::setw(16) << value;
}

#if defined(SILICA_REFERENCE)
void run_probe() {
  static silica::Cpu cpu(16u * 1024u * 1024u);
  cpu.regs.x[3] = kDataBase;
  cpu.regs.x[9] = 0xAAAAAAAA00000007ull;
  cpu.regs.x[11] = 0xDEADBEEF00000006ull;
  cpu.regs.x[12] = 0xFFFFFFFF00000005ull;
  cpu.mem.write<uint64_t>(kDataBase, 0x1122334455667788ull);
  if (!cpu.load_program(kCodeBase, std::span<const uint32_t>(kProgram))) {
    std::cerr << "reference: program does not fit memory\n";
    std::exit(2);
  }

  for (size_t step = 0; step < kExecutedSteps; ++step) {
    cpu.step();
    if (cpu.halted() || cpu.mem.fault()) {
      std::cerr << "reference halted/faulted at step " << step
                << " reason=" << static_cast<unsigned>(cpu.halt) << "\n";
      std::exit(3);
    }
    emit_u64(cpu.regs.pc);
    std::cout << ' ';
    emit_u64(cpu.regs.sp);
    std::cout << ' ';
    emit_u64(cpu.regs.pstate.nzcv());
    for (unsigned r = 0; r < 31; ++r) {
      std::cout << ' ';
      emit_u64(cpu.regs.x[r]);
    }
    std::cout << ' ';
    emit_u64(cpu.mem.read<uint64_t>(kDataBase));
    std::cout << '\n';
  }
}
#elif defined(SILICA_NEURO)
void run_probe() {
  static silica::Cpu cpu;
  cpu.reset();
  cpu.regs.x[3] = kDataBase;
  cpu.regs.x[1] = 0xDEADBEEF00000006ull;
  cpu.regs.x[2] = 0xFFFFFFFF00000005ull;
  cpu.regs.x[4] = 0xAAAAAAAA00000007ull;
  cpu.mem.store64(kDataBase, 0x1122334455667788ull);
  for (size_t i = 0; i < kProgram.size(); ++i) {
    cpu.mem.store32(kCodeBase + i * 4, kProgram[i]);
  }
  cpu.regs.pc = kCodeBase;

  for (size_t step = 0; step < kExecutedSteps; ++step) {
    cpu.run(1);
    if (cpu.mem.fault() != silica::FaultReason::None || cpu.yielded) {
      std::cerr << "neuro halted/faulted at step " << step
                << " reason=" << static_cast<unsigned>(cpu.mem.fault())
                << " yielded=" << cpu.yielded << "\n";
      std::exit(3);
    }
    emit_u64(cpu.regs.pc);
    std::cout << ' ';
    emit_u64(cpu.regs.sp);
    std::cout << ' ';
    emit_u64(cpu.regs.nzcv);
    for (unsigned r = 0; r < 31; ++r) {
      std::cout << ' ';
      emit_u64(cpu.regs.x[r]);
    }
    std::cout << ' ';
    emit_u64(cpu.mem.load64(kDataBase));
    std::cout << '\n';
  }
}
#endif
}  // namespace

int main() {
  run_probe();
  return 0;
}
