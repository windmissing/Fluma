#pragma once
#include <array>
#include <cstdint>
#include <limits>
namespace grs::g9dm {
inline bool CheckedAdd(uint64_t a,uint64_t b,uint64_t& out){if(b>UINT64_MAX-a)return false;out=a+b;return true;}
struct WideUnsigned {
    uint64_t low=0,high=0;
    static WideUnsigned Multiply(uint64_t value,uint32_t factor){
        const uint64_t lower=(value&0xffffffffULL)*factor;
        const uint64_t upper=(value>>32)*factor;
        const uint64_t low=lower+(upper<<32);
        return {low,(upper>>32)+(low<lower?1u:0u)};
    }
    bool Add(uint64_t value){const auto next=low+value;const auto carry=next<low?1u:0u;if(carry&&high==UINT64_MAX)return false;low=next;high+=carry;return true;}
    bool operator==(const WideUnsigned& b)const{return low==b.low&&high==b.high;}
};
struct ControlIdentity {
    uint64_t run=0,owner=0,context=0,inputEpoch=0,pageSession=0,permission=0,window=0,config=0,surface=0;
    bool operator==(const ControlIdentity& b)const{return run==b.run&&owner==b.owner&&context==b.context&&inputEpoch==b.inputEpoch&&pageSession==b.pageSession&&permission==b.permission&&window==b.window&&config==b.config&&surface==b.surface;}
    bool operator!=(const ControlIdentity& b)const{return !(*this==b);}
};
struct PagePulseV3 {ControlIdentity identity;uint64_t sequence=0,uiNs=0;};
struct SourceIntentKey {uint64_t run=0,owner=0,context=0,inputEpoch=0,pageSession=0,sourceControlGeneration=0,userIntentSequence=0;};
struct InputPacket {std::array<uint32_t,128> words{};};
enum class PlanningMode:uint32_t {RUNNABLE,BLOCKED_WAIT,REARM_PENDING,REVOKED};
enum class GravityMode:uint32_t {MANUAL,SENSOR};
enum class ControlResult:uint32_t {OK,STALE_REJECTED,SEQUENCE_REJECTED,ALREADY_RETIRED,WOULD_BLOCK,INVALID_ARGUMENT,CONTROL_CLOCK_REVERSED,CLOCK_GAP,QUOTA_OVERFLOW,DEADLINE,UNSUPPORTED};
struct ControlFault {ControlResult reason=ControlResult::STALE_REJECTED;};
struct PlanGrantV3 {
    uint64_t id=0,grantedNs=0,carry=0,session=0,sourceBoundary=0,frozenInputSeq=0;
    uint32_t quanta=0;int32_t target=0;
};
struct RunControl {
    bool active=false,sourcePhase=false,jobLive=false,workspaceRetired=true;
    uint32_t quantumOrdinal=0,continuationStage=0;
    uint64_t plan=0,nonce=0;
    bool RetireBlockedPlanLocked(uint64_t id,uint64_t root){if(!active||plan!=id||nonce!=root)return false;active=sourcePhase=jobLive=false;quantumOrdinal=continuationStage=0;plan=nonce=0;return true;}
};
using QuotaSnapshotV3=std::array<uint32_t,256>;
struct RecoveryHandoff {ControlIdentity identity;};
struct RetiredRunHandoff {ControlIdentity identity;};
}
