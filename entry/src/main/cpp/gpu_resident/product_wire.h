#pragma once
#include "control.h"
#include <array>
namespace grs {
bool ValidateSandStateWord(uint32_t);
bool ValidateEventWords(const uint32_t* event,size_t length,uint32_t& offendingWord);
// This only validates frozen wire shape/domains; physics replay is a separate GPU stage.
enum PassStage:uint32_t {Copy=0,PrepareSource,FreeFlags,ScanUp,ScanRoot,ScanDown,Birth,Fold,AuditFlags,AuditReduce,ReplayCopy,Replay,ReplayAudit,Provenance,StoreSource,Pack,PackAudit,Seal,PrepareQuantum,IndexInit,Sort,Proposal,Resolve,BedPropose,BedAllocate,BedApply,FinalContactFold};
struct DispatchPass {uint32_t stage=0,index=0,k=0,j=0,color=0;};
struct DispatchPlan {std::array<DispatchPass,68> passes{};uint32_t count=0;};
DispatchPlan SourcePlan(uint32_t K);DispatchPlan QuantumPlan(uint32_t K);
}
