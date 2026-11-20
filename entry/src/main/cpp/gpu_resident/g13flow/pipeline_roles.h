#pragma once
#include <cstddef>
#include <cstdint>

namespace grs::g13 {
struct PipelineRole {
  const char *name;
  const char *source;
  const char *sha;
};

inline constexpr size_t kMaxPipelineRoles = 16;
const PipelineRole *PipelineRoles();
size_t PipelineRoleCount();
size_t PipelineRoleForOpcode(uint32_t opcode);
const char *PipelineVariant();
} // namespace grs::g13
