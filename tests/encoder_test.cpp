#include "silica/enc.hpp"
#include <cstdint>

static_assert(silica::enc::add_imm(0, 1, 42) == 0x9100A820u);
static_assert(silica::enc::movz(1, 0x1234, 0, 2) == 0xD2824682u);
static_assert(silica::enc::b_imm(-4) == 0x17FFFFFCu);
static_assert(silica::enc::bl_imm(2) == 0x94000002u);
static_assert(silica::enc::b_cond(silica::enc::COND_NE, -1) == 0x54FFFFE1u);
static_assert(silica::enc::ret() == 0xD65F03C0u);
static_assert(silica::enc::ldr_x(0, 1, 8) == 0xF9400420u);
static_assert(silica::enc::str_x(0, 1, 8) == 0xF9000420u);
static_assert(silica::enc::nop() == 0xD503201Fu);
static_assert(silica::enc::brk(0) == 0xD4200000u);
static_assert(silica::enc::detail::in_range_s(-4, 26));
static_assert(!silica::enc::detail::in_range_s(1 << 25, 26));

// Variable-shift golden encodings.
static_assert(silica::enc::lslv(0, 1, 2) == 0x9AC22020u);
static_assert(silica::enc::lsrv(0, 1, 2) == 0x9AC22420u);
static_assert(silica::enc::asrv(0, 1, 2) == 0x9AC22820u);
static_assert(silica::enc::rorv(0, 1, 2) == 0x9AC22C20u);

// MRS/MSR NZCV golden encodings.
static_assert(silica::enc::mrs(0x5A10, 0) == 0xD53B4200u);
static_assert(silica::enc::msr(0x5A10, 0) == 0xD51B4200u);

// Safe bit extraction, including the full-width and invalid-range cases.
static_assert(silica::enc::detail::bits(0xDEADBEEFu, 0, 31) == 0xDEADBEEFu);
static_assert(silica::enc::detail::bits(0xDEADBEEFu, 4, 7) == 0xEu);
static_assert(silica::enc::detail::bits(0xDEADBEEFu, 8, 7) == 0u);
static_assert(silica::enc::detail::bits(0xDEADBEEFu, 0, 32) == 0u);

// Operand-validation helper boundaries.
static_assert(silica::enc::detail::is_valid_rd(31));
static_assert(!silica::enc::detail::is_valid_rd(32));
static_assert(silica::enc::detail::is_valid_imm12(0xFFF));
static_assert(!silica::enc::detail::is_valid_imm12(0x1000));
static_assert(silica::enc::detail::is_valid_imm16(0xFFFF));
static_assert(!silica::enc::detail::is_valid_imm16(0x10000));
static_assert(silica::enc::detail::is_valid_cond(14));
static_assert(!silica::enc::detail::is_valid_cond(15));

int main() { return 0; }
