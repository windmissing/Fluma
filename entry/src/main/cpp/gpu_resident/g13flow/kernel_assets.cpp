#include "contract_generated.h"
#include "pipeline_roles.h"
#include <pipeline_source.h>
namespace grs::g13 {
static_assert(sizeof(kG13Roles) / sizeof(kG13Roles[0]) <= kMaxPipelineRoles);
bool PipelineTestHooks() { return kG13TestHooks; }
const char *PipelineSource() { return kG13Roles[0].source; }
const char *PipelineSha() { return kG13PipelineSha; }
const PipelineRole *PipelineRoles() { return kG13Roles; }
size_t PipelineRoleCount() { return sizeof(kG13Roles) / sizeof(kG13Roles[0]); }
size_t PipelineRoleForOpcode(uint32_t opcode) {
  return opcode < sizeof(kG13OpcodeRole) ? kG13OpcodeRole[opcode]
                                        : PipelineRoleCount();
}
const char *PipelineVariant() { return kG13Variant; }
} // namespace grs::g13
