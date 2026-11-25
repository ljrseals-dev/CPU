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

// The branch at word 4 skips one decoy. BL calls the RET at word 16;
// return continues at word 14, then B skips the callee and reaches all
// three W-form instructions. Every declared instruction is executed once.
constexpr std::array<uint32_t, 20> kProgram{{
    0xD2800140u,  // MOVZ X0, #10
    0xD28000A1u,  // MOVZ X1, #5
    0x8B010000u,  // ADD X0, X0, X1
    0xF1003C1Fu,  // SUBS XZR, X0, #15
    0x54000040u,  // B.EQ +8
    0xD2800C62u,  // skipped: MOVZ X2, #99
    0xD28000E2u,  // MOVZ X2, #7
    0xF9000062u,  // STR X2, [X3]
    0xF9400064u,  // LDR X4, [X3]
    0x8B040045u,  // ADD X5, X2, X4
    0x9B021027u,  // MADD X7, X1, X2, X4
    0x9B029028u,  // MSUB X8, X1, X2, X4
    0xF10007FFu,  // SUBS XZR, SP, #1
    0x94000003u,  // BL +12; X30 receives address of word 14
    0xD280054Au,  // continuation: MOVZ X10, #42
    0x14000002u,  // B +8; skip the RET after returning
    0xD65F03C0u,  // callee: RET X30
    0x0B0C0D66u,  // ADD W6, W11, W12, LSL #3
    0x1B0C2527u,  // MADD W7, W11, W12, W9
    0x1B0CA528u,  // MSUB W8, W11, W12, W9
}};
constexpr std::size_t kExecutedSteps = 19;

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
  cpu.regs.x[9] = 0xAAAAAAAA00000007ull;
  cpu.regs.x[11] = 0xDEADBEEF00000006ull;
  cpu.regs.x[12] = 0xFFFFFFFF00000005ull;
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
