// T522 generated explicit V5 cold protocol; see generate_cold.py.
#pragma once
#include "../cold_slot_broker.h"
#include "cold_contract.h"
namespace grs {namespace g9spf {namespace coldtrace {
using cold503::Scope;using cold503::ScopeJson;
using Writer=cold::Writer;
inline constexpr size_t MaxTraceBytes=cold::kMaximumTraceBytes,TraceBudget=cold::kTwoSlotTraceBytes,PageWords=cold::kPageWords;
inline constexpr uint64_t TtlNs=60000000000ULL;
// All methods below require the single App control lock. Storage ownership is
// transferred explicitly; allocation/free/Capture/encoding occur outside it.
class TraceManager {
public:
 enum State {FREE,BUILDING,READY,RETIRING};
 struct Slot {
  coldbroker::Lease credit; // declared before block: delete[] precedes credit refund
  State state=FREE;std::unique_ptr<uint32_t[]> block;size_t capacity=0,reserved=0,stateWords=0,eventWords=0,expectedState=0,requestWords=0;uint32_t fullRows=0;
  uint64_t request=0,id=0,created=0,last=0;Scope scope{};bool reader=false,started=false,cancel=false;
  const char* result="FREE";uint32_t gl=0,egl=0,transportCode=0;std::array<char,48> failedStage{};
 };
 struct Receipt {uint64_t request=0,id=0;Scope scope{};const char* result="STALE_TRACE_ID";uint32_t gl=0,egl=0,transportCode=0;std::array<char,48> failedStage{};};
 struct Read {int slot=-1;uint64_t id=0;Scope scope{};const uint32_t* block=nullptr;size_t capacity=0,stateWords=0,eventWords=0;uint32_t fullRows=0;};
 std::array<Slot,2> slots;std::array<Receipt,32> terminal{};uint64_t terminalSeen=0,requestSerial=0,traceSerial=0;int published=-1,builder=-1;bool reading=false;
 uint64_t allocations=0,allocationFailures=0,readerBusy=0;size_t peakCapacity=0,detachedBytes=0;
 size_t Capacity()const{size_t n=detachedBytes;for(const auto& s:slots)n+=s.capacity*4;return n;}
 size_t Reserved()const{size_t n=0;for(const auto& s:slots)if(s.reserved>s.capacity)n+=(s.reserved-s.capacity)*4;return n;}
 Receipt Of(const Slot& s)const{return {s.request,s.id,s.scope,s.result,s.gl,s.egl,s.transportCode,s.failedStage};}
 void Remember(const Slot& s){terminal[terminalSeen++%terminal.size()]=Of(s);}
 const Receipt* History(uint64_t id,bool request=false)const{for(uint64_t i=0;i<std::min<uint64_t>(terminalSeen,terminal.size());++i){const auto& r=terminal[(terminalSeen-1-i)%terminal.size()];if((request?r.request:r.id)==id)return &r;}return nullptr;}
 void Close(int index,const char* why){
  auto& s=slots[index];s.result=why;s.cancel=true;Remember(s);if(published==index)published=-1;
  if(s.state==BUILDING)return;s.state=s.reader?RETIRING:FREE;
 }
 void Maintain(uint64_t now,const Scope& scope){
  for(int i=0;i<2;++i){auto& s=slots[i];if(s.state==FREE||s.state==RETIRING||s.cancel)continue;
   if(s.scope!=scope)Close(i,"STALE_SCOPE");else if(now<s.last)Close(i,"CLOCK_REVERSED");else if(now-s.created>=TtlNs)Close(i,"EXPIRED");else s.last=now;
  }
 }
 const char* Begin(const Scope& expected,const Scope& actual,uint64_t now,uint64_t replace,size_t words,uint64_t& request,uint64_t& id,size_t stateWords,uint32_t fullRows=0){
  Maintain(now,actual);request=id=0;if(expected!=actual)return "STALE_SCOPE";if(std::strcmp(actual.route.data(),"g9spf"))return "UNSUPPORTED_ROUTE";
  if(words<256||words>(fullRows?cold::kMaximumFullTraceBytes:MaxTraceBytes)/4||!stateWords||stateWords>words-256)return "TRACE_FORMAT_REJECTED";if(builder>=0)return "WOULD_BLOCK";
  if(published>=0&&replace!=slots[published].id)return "TRACE_BUSY";if(published<0&&replace)return "STALE_TRACE_ID";
  if(requestSerial==UINT64_MAX||traceSerial==UINT64_MAX)return "ID_EXHAUSTED";
  int free=-1;for(int i=0;i<2;++i)if(slots[i].state==FREE&&!slots[i].reader){free=i;if(slots[i].capacity>=words)break;}
  if(free<0)return "WOULD_BLOCK";auto& s=slots[free];const auto target=std::max(words,s.capacity);
  if(Capacity()-s.capacity*4+target*4>(fullRows?cold::kTwoSlotFullTraceBytes:TraceBudget))return "WOULD_BLOCK";
  uint64_t globalRequest=0,globalTrace=0;if(!s.credit.Reserve(target*4,fullRows?cold::kTwoSlotFullTraceBytes:TraceBudget,9,globalRequest,globalTrace))return "WOULD_BLOCK";
  // Old free capacity remains charged until Taken block is actually freed.
  s.reserved=s.capacity>=words?0:words;s.state=BUILDING;s.started=s.cancel=false;s.result="REQUESTED";s.request=globalRequest;s.id=globalTrace;++requestSerial;++traceSerial;s.scope=actual;s.created=s.last=now;s.stateWords=s.eventWords=0;s.expectedState=stateWords;s.fullRows=fullRows;s.requestWords=words;s.gl=s.egl=s.transportCode=0;s.failedStage={};builder=free;request=s.request;id=s.id;return "REQUESTED";
 }
 int Start(){if(builder<0)return -1;auto& s=slots[builder];if(s.started)return -1;if(s.cancel){s.credit.BuilderDone();if(!s.block)s.credit.Reset();s.state=FREE;s.reserved=0;builder=-1;return -1;}s.started=true;s.result="BUILDING";return builder;}
 std::unique_ptr<uint32_t[]> TakeOld(int i){auto& s=slots[i];if(!s.reserved)return {};return std::move(s.block);}
 void OldFreed(int i){if(slots[i].reserved)slots[i].capacity=0;}
 void Adopt(int i,std::unique_ptr<uint32_t[]> block){auto& s=slots[i];if(s.reserved){s.block=std::move(block);s.capacity=s.block?s.reserved:0;s.reserved=0;if(s.block)++allocations;else ++allocationFailures;peakCapacity=std::max(peakCapacity,Capacity());}}
 static bool Layout(const Slot& s,size_t state,size_t events){
  if(!s.block||s.capacity<s.requestWords||state+256!=s.requestWords||events!=0)return false;
  const auto* b=s.block.get();const auto* p=SelectProfile(b[7]);cold::Layout layout;
  if(!p||!cold::Layout::Make(*p,4,layout,s.fullRows!=0)||layout.words!=s.requestWords)return false;
  return cold::Validate(b,s.capacity,layout);
 }
 void Finish(int i,const Scope& actual,uint64_t now,const char* result,size_t state,size_t events,uint32_t gl=0,uint32_t egl=0,const char* stage="",uint32_t transportCode=0){
  Maintain(now,actual);auto& s=slots[i];s.credit.BuilderDone();if(!s.block)s.credit.Reset();s.gl=gl;s.egl=egl;s.transportCode=transportCode;std::strncpy(s.failedStage.data(),stage,s.failedStage.size()-1);
  if(!s.cancel&&std::strcmp(result,"OK")==0&&!Layout(s,state,events))result="TRACE_FORMAT_REJECTED";
  if(s.cancel||std::strcmp(result,"OK")){if(!s.cancel)s.result=result;Remember(s);s.state=FREE;s.reserved=0;builder=-1;return;}
  const int old=published;s.state=READY;s.result="TRACE_READY";s.stateWords=state;s.eventWords=events;published=i;builder=-1;
  if(old>=0&&old!=i)Close(old,"REPLACED");
 }
 Receipt StateOf(uint64_t request,uint64_t now,const Scope& actual){Maintain(now,actual);for(const auto& s:slots)if(s.request==request&&s.state!=FREE)return Of(s);if(auto* r=History(request,true))return *r;Receipt out;out.request=request;out.result="STALE_REQUEST_ID";return out;}
 const char* End(uint64_t id,uint64_t now,const Scope& actual){Maintain(now,actual);for(int i=0;i<2;++i)if(slots[i].id==id&&slots[i].state!=FREE){if(slots[i].cancel)return slots[i].result;Close(i,"ENDED");return "ENDED";}if(auto* r=History(id))return r->result;return "STALE_TRACE_ID";}
 const char* Acquire(uint64_t id,uint32_t page,uint64_t now,const Scope& actual,Read& out){
  Maintain(now,actual);if(reading){++readerBusy;return "COLD_READER_BUSY";}
  for(int i=0;i<2;++i){auto& s=slots[i];if(s.id!=id||!id)continue;if(s.state==FREE)break;if(s.state==BUILDING&&!s.cancel)return "TRACE_NOT_READY";if(s.state!=READY||s.cancel)return s.result;
   const auto* p=SelectProfile(s.block[7]);cold::Layout layout;
   if(!p||!cold::Layout::Make(*p,4,layout,s.fullRows!=0))return "TRACE_FORMAT_REJECTED";
   const size_t pages=layout.Pages();if(page>=pages)return "TRACE_BAD_PAGE";if(!s.credit.ReadBegin()){++readerBusy;return "COLD_READER_BUSY";}s.reader=true;reading=true;out={i,id,s.scope,s.block.get(),s.capacity,s.stateWords,s.eventWords,s.fullRows};return "OK";
  }if(auto* r=History(id))return r->result;return "STALE_TRACE_ID";
 }
 const char* Release(const Read& read,uint64_t now,const Scope& actual){Maintain(now,actual);auto& s=slots[read.slot];s.credit.ReadEnd();s.reader=false;reading=false;const auto result=s.result;if(s.state==RETIRING)s.state=FREE;return result;}
 coldbroker::DetachedBlock Trim(int i){auto& s=slots[i];if(s.state!=FREE||s.reader||s.reserved||!s.block)return {};detachedBytes+=s.capacity*4;s.capacity=0;return {std::move(s.credit),std::move(s.block)};}
 // Caller must reset/destroy the detached unique_ptr before this credit is
 // returned; moving ownership out of a slot is not a free acknowledgment.
 bool DetachedFreed(size_t bytes){if(bytes>detachedBytes)return false;detachedBytes-=bytes;return true;}
};
inline void ReceiptJson(Writer& w,const TraceManager::Receipt& r,bool real=false){w.Raw(real?"{\"revision\":\"G9_SPF_COLD_V2\",\"state\":" : "{\"revision\":\"G9_SPF_COLD_V1\",\"state\":");w.String(r.result,std::strlen(r.result));w.Raw(",\"requestId\":");w.U64(r.request);w.Raw(",\"traceId\":");w.U64(r.id);w.Raw(",\"scope\":");ScopeJson(w,r.scope);w.Raw(",\"rawGl\":");w.Number(r.gl);w.Raw(",\"rawEgl\":");w.Number(r.egl);w.Raw(",\"transportCode\":");w.Number(r.transportCode);w.Raw(",\"failedStage\":");w.String(r.failedStage);w.Raw("}");}
inline void TracePage(Writer& w,const TraceManager::Read& read,uint32_t page){
 const auto* p=SelectProfile(read.block[7]);cold::Layout layout;
 if(!p||!cold::Layout::Make(*p,4,layout,read.fullRows!=0)||!cold::Page(w,read.block,layout,read.id,read.scope,page)){
  w.Reset();w.Raw("{\"state\":\"INCOMPLETE\",\"reason\":\"TRACE_FORMAT_REJECTED\"}");
 }
}
}}}
