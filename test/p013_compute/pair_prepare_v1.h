#pragma once
#include "pair_wire_v1.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace p011gpu {
enum class PrepareStatus {OK,NO_PAIR,INPUT_INVALID,CAPACITY_UNSUPPORTED,ENV_UNSUPPORTED};
struct SnapshotView {
 uint32_t nmax=0,cols=0,liveCount=0,freeCount=0;double nextId=0;
 const double *mass=nullptr,*x=nullptr,*y=nullptr,*vx=nullptr,*vy=nullptr;
 const uint8_t *phase=nullptr,*source=nullptr,*state=nullptr;
 const uint32_t *id=nullptr,*sourceId=nullptr;
 const int32_t *freeSlots=nullptr;const double *colMassH=nullptr,*colMassL=nullptr;
 std::array<size_t,13> byteLength{}; // schema order mass,x,y,vx,vy,phase,source,state,id,sourceId,freeSlots,colH,colL
};
struct RequestIdentity {
 std::string runId,stateId,configId,buildManifestSha;
 std::string identityEncodingRevision="T141_LEGACY_IDENTITY_ORDER_V1";
 uint64_t ownerGeneration=0,contextEpoch=0,payloadSerial=0,poolMutationSerial=0;
 uint64_t attemptSerial=0,substepSerial=0,requestSerial=0;
 uint32_t candidateRole=0,candidateOrdinal=0,slotIndex=0,flags=0;
 double paramsRevision=0,inputRevision=0,seed=0;
};
struct CpuSlot {
 uint32_t nmax=0,K=0,h=0;std::vector<uint32_t> order,idOrder;
 std::vector<Leaf> leaves;std::vector<Query> queries;
 std::vector<double> fMass,fX,fY,fVx,fVy,fColH,fColL;
 std::vector<uint8_t> fPhase,fSource,fState;
 std::vector<uint32_t> fId,fSourceId;std::vector<int32_t> fFree;
 std::vector<uint8_t> freeSeen;
 SnapshotView frozen;
 std::vector<uint8_t> identityBytes;
 Header header;Result result;uint32_t n=0;bool prepared=false;
 explicit CpuSlot(uint32_t capacity);
};
struct PreparedView {const Leaf* leaves=nullptr;const Query* queries=nullptr;const Header* header=nullptr;
 uint32_t n=0,K=0,h=0;double radius=0;RequestIdentity identity;
 const SnapshotView* snapshot=nullptr;uint64_t mutationAtFreeze=0;
};
struct PrepareTimingV3 { double validate=-1,freeze=-1,sort=-1,range=-1,hashPack=-1; };
PrepareStatus prepareExactRanges(const SnapshotView&,double,const RequestIdentity&,CpuSlot&,PreparedView&,PrepareTimingV3* timing=nullptr);
// Test-side authority only; no pool write. Every result is checked again by owner.
bool legacyPair(const SnapshotView&,double,uint32_t& slotA,uint32_t& slotB);
bool sameGroup(const SnapshotView&,uint32_t,uint32_t);
}
