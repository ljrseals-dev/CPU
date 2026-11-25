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

int main() { return 0; }
