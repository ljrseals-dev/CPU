#pragma once
// silica/enc.hpp
// Pure constexpr AArch64 instruction encoders.
// Independent of the CPU core, decoder, and semantics.

#include <cstdint>

namespace silica::enc {

using u = uint32_t;

namespace detail {
constexpr bool in_range_u(u value, unsigned bits) noexcept {
  return bits >= 32 || value < (u{1} << bits);
}
constexpr bool in_range_s(int32_t value, unsigned bits) noexcept {
  if (bits == 0 || bits > 32) return false;
  if (bits == 32) return true;
  const int64_t limit = int64_t{1} << (bits - 1);
  return value >= -limit && value < limit;
}
constexpr u mask(unsigned bits) noexcept {
  return bits >= 32 ? ~u{0} : (bits == 0 ? 0u : (u{1} << bits) - 1u);
}
} // namespace detail

// ADD/SUB (immediate). sf=1 selects X registers; sh=1 shifts imm12 by 12.
constexpr u add_sub_imm(u sf, u sub, u s, u sh, u imm12, u rn, u rd) noexcept {
  return ((sf & 1u) << 31) | ((sub & 1u) << 30) |
         ((s & 1u) << 29) | 0x11000000u |
         ((sh & 1u) << 22) | ((imm12 & 0xFFFu) << 10) |
         ((rn & 0x1Fu) << 5) | (rd & 0x1Fu);
}
constexpr u add_imm(u rd, u rn, u imm12, u sf = 1) noexcept {
  return add_sub_imm(sf, 0, 0, 0, imm12, rn, rd);
}
constexpr u adds_imm(u rd, u rn, u imm12, u sf = 1) noexcept {
  return add_sub_imm(sf, 0, 1, 0, imm12, rn, rd);
}
constexpr u sub_imm(u rd, u rn, u imm12, u sf = 1) noexcept {
  return add_sub_imm(sf, 1, 0, 0, imm12, rn, rd);
}
constexpr u subs_imm(u rd, u rn, u imm12, u sf = 1) noexcept {
  return add_sub_imm(sf, 1, 1, 0, imm12, rn, rd);
}

// MOVN/MOVZ/MOVK, opc = 0/2/3.
constexpr u mov_wide(u opc, u sf, u imm16, u hw, u rd) noexcept {
  return ((sf & 1u) << 31) | ((opc & 3u) << 29) | 0x12800000u |
         ((hw & 3u) << 21) | ((imm16 & 0xFFFFu) << 5) | (rd & 0x1Fu);
}
constexpr u movn(u sf, u imm16, u hw, u rd) noexcept { return mov_wide(0, sf, imm16, hw, rd); }
constexpr u movz(u sf, u imm16, u hw, u rd) noexcept { return mov_wide(2, sf, imm16, hw, rd); }
constexpr u movk(u sf, u imm16, u hw, u rd) noexcept { return mov_wide(3, sf, imm16, hw, rd); }

// Branch offsets are signed counts of 4-byte instructions.
constexpr u b_imm(int32_t words) noexcept {
  return 0x14000000u | (static_cast<u>(words) & 0x03FFFFFFu);
}
constexpr u bl_imm(int32_t words) noexcept {
  return 0x94000000u | (static_cast<u>(words) & 0x03FFFFFFu);
}
constexpr u b_cond(u cond, int32_t words) noexcept {
  return 0x54000000u | ((static_cast<u>(words) & 0x7FFFFu) << 5) | (cond & 0xFu);
}
constexpr u cbz_cbnz(u sf, u nz, int32_t words, u rt) noexcept {
  return ((sf & 1u) << 31) | 0x34000000u | ((nz & 1u) << 24) |
         ((static_cast<u>(words) & 0x7FFFFu) << 5) | (rt & 0x1Fu);
}
constexpr u cbz(u sf, int32_t words, u rt) noexcept { return cbz_cbnz(sf, 0, words, rt); }
constexpr u cbnz(u sf, int32_t words, u rt) noexcept { return cbz_cbnz(sf, 1, words, rt); }

constexpr u br(u rn) noexcept { return 0xD61F0000u | ((rn & 31u) << 5); }
constexpr u blr(u rn) noexcept { return 0xD63F0000u | ((rn & 31u) << 5); }
constexpr u ret(u rn = 30) noexcept { return 0xD65F0000u | ((rn & 31u) << 5); }

// Unsigned-offset load/store. size: B/H/W/X = 0/1/2/3;
// opc: store/load/sign-extend-to-X/sign-extend-to-W = 0/1/2/3.
constexpr u ldst_uimm(u size, u opc, u imm12, u rn, u rt) noexcept {
  return ((size & 3u) << 30) | 0x39000000u | ((opc & 3u) << 22) |
         ((imm12 & 0xFFFu) << 10) | ((rn & 31u) << 5) | (rt & 31u);
}
constexpr u ldr_x(u rt, u rn, u byte_offset = 0) noexcept {
  return ldst_uimm(3, 1, byte_offset >> 3, rn, rt);
}
constexpr u str_x(u rt, u rn, u byte_offset = 0) noexcept {
  return ldst_uimm(3, 0, byte_offset >> 3, rn, rt);
}

constexpr u nop() noexcept { return 0xD503201Fu; }
constexpr u brk(u imm16 = 0) noexcept { return 0xD4200000u | ((imm16 & 0xFFFFu) << 5); }

constexpr u COND_EQ = 0, COND_NE = 1, COND_CS = 2, COND_CC = 3;
constexpr u COND_MI = 4, COND_PL = 5, COND_VS = 6, COND_VC = 7;
constexpr u COND_HI = 8, COND_LS = 9, COND_GE = 10, COND_LT = 11;
constexpr u COND_GT = 12, COND_LE = 13, COND_AL = 14;

} // namespace silica::enc
