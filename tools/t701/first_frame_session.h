#pragma once
#include "control.h"
#include <sstream>

namespace grs::t717 {
inline uint64_t Wide(U64 word) { return uint64_t(word.hi) << 32 | word.lo; }

struct Scope {
  uint64_t epoch=0, run=0, owner=0, context=0, surface=0;
  uint64_t permission=0, session=0, window=0, ownerTid=0;

  void Json(std::ostringstream& out) const {
    out<<"{\"epoch\":\""<<epoch<<"\",\"run\":\""<<run<<"\",\"owner\":\""<<owner
       <<"\",\"runLo\":"<<uint32_t(run)<<",\"runHi\":"<<uint32_t(run>>32)
       <<",\"context\":\""<<context<<"\",\"surface\":\""<<surface
       <<"\",\"permissionRevision\":\""<<permission<<"\",\"session\":\""<<session
       <<"\",\"window\":\""<<window<<"\",\"ownerTid\":\""<<ownerTid<<"\"}";
  }
};

// One terminal pulse witness, the first accepted root, and the first swap
// attempt per native epoch/session. All updates use the existing App mutex.
struct Evidence {
  struct Pulse {
    Scope scope{};
    uint64_t token=0, seq=0, callback=0, receive=0;
    uint64_t previousSeq=0, previousCallback=0, previousReceive=0;
    bool openBefore=false, openAfter=false;
    const char* result="";
    const char* before="";
    const char* after="";
    const char* phase="";
    bool observed=false;
  } rejection;
  struct Root {
    Scope scope{};
    uint64_t nonce=0, accepted=0, physics=0, ns=0;
  } firstRoot;
  struct Swap {
    Scope scope{};
    uint64_t nonce=0, begin=0, nativeBegin=0, end=0;
    int code=-1;
    bool called=false, returned=false, nativeSucceeded=false;
  } firstSwap;

  void Json(std::ostringstream& out) const {
    const auto& p=rejection;
    out<<",\"firstFrameSession\":{\"pulse\":";
    if(!p.observed)out<<"null";
    else {
      out<<"{\"scope\":";p.scope.Json(out);
      out<<",\"token\":\""<<p.token<<"\",\"seq\":\""<<p.seq
         <<"\",\"callbackNs\":\""<<p.callback<<"\",\"receiveNs\":\""<<p.receive
         <<"\",\"previousSeq\":\""<<p.previousSeq<<"\",\"previousCallbackNs\":\""<<p.previousCallback
         <<"\",\"previousReceiveNs\":\""<<p.previousReceive<<"\",\"openBefore\":"<<(p.openBefore?"true":"false")
         <<",\"openAfter\":"<<(p.openAfter?"true":"false")<<",\"result\":\""<<p.result
         <<"\",\"faultBefore\":\""<<p.before<<"\",\"faultAfter\":\""<<p.after
         <<"\",\"phase\":\""<<p.phase<<"\"}";
    }
    out<<",\"firstRoot\":";
    if(!firstRoot.ns)out<<"null";
    else {
      out<<"{\"scope\":";firstRoot.scope.Json(out);
      out<<",\"nonce\":\""<<firstRoot.nonce<<"\",\"acceptedCount\":\""<<firstRoot.accepted
         <<"\",\"physicsCommit\":\""<<firstRoot.physics<<"\",\"observedNs\":\""<<firstRoot.ns<<"\"}";
    }
    out<<",\"firstSwap\":";
    if(!firstSwap.begin)out<<"null";
    else {
      out<<"{\"scope\":";firstSwap.scope.Json(out);
      out<<",\"nonce\":\""<<firstSwap.nonce<<"\",\"beginNs\":\""<<firstSwap.begin
         <<"\",\"nativeBeginNs\":\""<<firstSwap.nativeBegin
         <<"\",\"endNs\":\""<<firstSwap.end<<"\",\"code\":"<<firstSwap.code
         <<",\"called\":"<<(firstSwap.called?"true":"false")<<",\"returned\":"<<(firstSwap.returned?"true":"false")
         <<",\"nativeSucceeded\":"<<(firstSwap.nativeSucceeded?"true":"false")<<"}";
    }
    out<<'}';
  }
};
} // namespace grs::t717
