#pragma once
#include "pair_prepare_v1.h"
#include <string>

namespace p011gpu {
enum class ConsumeStatus {OK,NO_PAIR,STALE,INPUT_INVALID,PROTOCOL_ERROR,ORACLE_MISMATCH};
struct CurrentOwnerState {
 RequestIdentity identity;const SnapshotView* current=nullptr;uint64_t poolMutationSerial=0;
 bool active=false;bool diagnosticOracle=true;
};
struct VerifiedPair {bool found=false;uint32_t idA=0,idB=0,slotA=0,slotB=0,aRank=0,bRank=0;};
ConsumeStatus verifyAndConsume(const PreparedView&,const Result&,const CurrentOwnerState&,VerifiedPair&);
bool sameSnapshotBytes(const SnapshotView&,const SnapshotView&);
}
