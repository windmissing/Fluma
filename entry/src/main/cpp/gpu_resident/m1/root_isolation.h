#pragma once
#include "../control.h"
#include "../submit_observer.h"

namespace grs {namespace m1 {
enum class RootCut:uint32_t {Stage=1, Dispatch, Flush, Fence, Wait, Map, Audit, Commit, Swap, Batch};
inline uint64_t NonceOf(const RootRequest& r){return (uint64_t(r.nonce.hi)<<32)|r.nonce.lo;}

// All callbacks run synchronously on the GL owner. Only Commit enters a
// short control transaction and its body may not issue any GL command.
struct RootIsolation {
 void* context=nullptr;
 bool (*authorize)(void*,const RootRequest&,RootCut)=nullptr;
 bool (*accept)(void*,const RootRequest&,const AcceptedRoot&,void*,void (*)(void*))=nullptr;
 uint64_t (*stamp)(void*)=nullptr;
 uint64_t deadline=0,revision=0;
 TailObserver observer;
 bool Check(const RootRequest& r,RootCut c)const{return !authorize||authorize(context,r,c);}
 template<class F>
 bool Commit(const RootRequest& r,const AcceptedRoot& out,F& body)const{
  if(!accept){body();return true;}
  return accept(context,r,out,&body,[](void* arg){(*static_cast<F*>(arg))();});
 }
 uint64_t Stamp()const{return observer.write?(stamp?stamp(context):TailObserver::Now()):0;}
 void Record(const RootRequest& r,RootCut c,uint64_t begin,uint32_t code)const{
  if(!observer.write)return;
  const auto end=Stamp();
  const auto remain=[&](uint64_t time){return deadline>time?deadline-time:0;};
  observer.Emit({TailStage::IsolationCall,NonceOf(r),begin,end,remain(begin),remain(end),revision,code,uint32_t(c)});
 }
};
}}
