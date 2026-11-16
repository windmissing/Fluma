#pragma once
#include <array>
#include <cstddef>
namespace grs::m3a {
// Same stage code and fixed table, specialized only by the public opcode.
// Host never specializes an angle, preview result or physics decision. The
// cache key includes this exact prefix; program/JIT memory is a separate ledger.
inline constexpr std::array<const char*,15> kOpcodeDefines={
 "#define M3_FIXED_OPCODE 0u\n","#define M3_FIXED_OPCODE 1u\n","#define M3_FIXED_OPCODE 2u\n",
 "#define M3_FIXED_OPCODE 3u\n","#define M3_FIXED_OPCODE 4u\n","#define M3_FIXED_OPCODE 5u\n",
 "#define M3_FIXED_OPCODE 6u\n","#define M3_FIXED_OPCODE 7u\n","#define M3_FIXED_OPCODE 8u\n",
 "#define M3_FIXED_OPCODE 9u\n","#define M3_FIXED_OPCODE 10u\n","#define M3_FIXED_OPCODE 11u\n",
 "#define M3_FIXED_OPCODE 12u\n","#define M3_FIXED_OPCODE 13u\n","#define M3_FIXED_OPCODE 14u\n"};
inline constexpr size_t kMaximumKernelSourceBytes=256*1024;
}
