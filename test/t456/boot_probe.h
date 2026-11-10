#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <ostream>
#include <cstring>

namespace grs::boot456 {
struct Point {
 const char* stage="";
 uint64_t ns=0,tid=0,root=0,a=0,b=0,c=0;
 int64_t result=0;
 uint32_t rawEgl=0,rawGl=0;
};
struct Transition {
 const char* reason="";
 uint64_t ns=0,tid=0,before=0,after=0,surface=0;
 uint64_t a=0,b=0;
 int64_t result=0;
};
struct Attempt {
 uint64_t serial=0,epoch=0,surface=0,window=0,candidateRun=0,publishedRun=0,readyEpoch=0,session=0,ownerTid=0;
 uint32_t k=0,kw=0,width=0,height=0;
 bool v2=false,terminal=false,drawDone=false,quantumDone=false;
 std::array<char,512> shaderLog{};
 uint64_t firstRoot[2]{};
 uint32_t dispatchSeen[2]{};
 Point firstFailure{},latest{};
 std::array<Point,256> points{};
 size_t count=0;
 uint64_t dropped=0;
};
struct Snapshot {
 std::array<Attempt,6> attempts{};
 std::array<Transition,64> transitions{};
 uint64_t attemptsSeen=0,transitionsSeen=0;
};
class Probe {
 std::mutex mutex;
 Snapshot data;
 Attempt* find(uint64_t epoch){
  for(size_t i=0;i<data.attempts.size()&&i<data.attemptsSeen;++i){
   auto& row=data.attempts[(data.attemptsSeen-1-i)%data.attempts.size()];
   if(row.epoch==epoch)return &row;
  }
  return nullptr;
 }
 static void add(Attempt& a,const Point& p,bool failure){
  if(failure&&a.firstFailure.stage[0]=='\0')a.firstFailure=p;
  if(!failure&&std::strncmp(p.stage,"WAIT_",5)==0&&std::strcmp(p.stage,a.latest.stage)==0)return;
  a.latest=p;
  if(a.count<a.points.size())a.points[a.count++]=p;else ++a.dropped;
 }
public:
 void Begin(uint64_t epoch,uint64_t surface,bool window,uint64_t tid,uint32_t k,uint32_t kw,bool v2,uint64_t ns){
  std::lock_guard<std::mutex> lock(mutex);
  if(find(epoch))return;
  auto& a=data.attempts[data.attemptsSeen%data.attempts.size()];a={};
  a.serial=++data.attemptsSeen;a.epoch=epoch;a.surface=surface;a.window=window?surface:0;
  a.ownerTid=tid;a.k=k;a.kw=kw;a.v2=v2;add(a,{"OWNER_ATTEMPT",ns,tid},false);
 }
 void Identity(uint64_t epoch,uint64_t run,uint64_t published,uint64_t ready,uint64_t session,uint32_t w,uint32_t h){
  std::lock_guard<std::mutex> lock(mutex);if(auto* a=find(epoch)){
   if(run)a->candidateRun=run;
   a->publishedRun=published;a->readyEpoch=ready;a->session=session;a->width=w;a->height=h;
  }
 }
 void Mark(uint64_t epoch,const Point& p,bool failure=false,bool terminal=false){
  std::lock_guard<std::mutex> lock(mutex);if(auto* a=find(epoch)){add(*a,p,failure);a->terminal|=terminal;}
 }
 void TransitionAt(const Transition& t,bool revoke=false){
  std::lock_guard<std::mutex> lock(mutex);
  data.transitions[data.transitionsSeen%data.transitions.size()]=t;++data.transitionsSeen;
  if(revoke&&t.before!=t.after)if(auto* a=find(t.before)){
   add(*a,{t.reason,t.ns,t.tid,0,t.before,t.after,t.surface,t.result},true);a->terminal=true;
  }
 }
 void Intent(uint64_t epoch,uint32_t kind,const Point& p){
  if(kind<1||kind>2||!p.root)return;
  std::lock_guard<std::mutex> lock(mutex);if(auto* a=find(epoch)){
   auto& root=a->firstRoot[kind-1];if(!root){root=p.root;add(*a,p,false);}
  }
 }
 void Root(uint64_t epoch,const Point& p){
  std::lock_guard<std::mutex> lock(mutex);if(auto* a=find(epoch)){
   if(a->terminal)return;
   if(p.root&&p.root!=a->firstRoot[0]&&p.root!=a->firstRoot[1])return;
   if(!p.root&&a->firstRoot[1])return;
   if(std::strcmp(p.stage,"SUBMIT_OBSERVER")==0&&(p.a==34||p.a==35)){
    const unsigned slot=p.root==a->firstRoot[1]?1:0;
    const uint32_t bit=1u<<uint32_t(p.a-34);
    if(a->dispatchSeen[slot]&bit){a->latest=p;return;}
    a->dispatchSeen[slot]|=bit;
   }
   add(*a,p,false);
   if(p.root&&p.root==a->firstRoot[1]&&std::strcmp(p.stage,"ROOT_ACCEPT")==0){a->quantumDone=true;a->terminal|=a->drawDone;}
  }
 }
 void Draw(uint64_t epoch,const Point& p,bool failure){
  std::lock_guard<std::mutex> lock(mutex);if(auto* a=find(epoch)){if(a->drawDone)return;add(*a,p,failure);a->drawDone=true;a->terminal|=failure||a->quantumDone;}
 }
 void ShaderLog(uint64_t epoch,const char* text){
  std::lock_guard<std::mutex> lock(mutex);if(auto* a=find(epoch))if(!a->shaderLog[0]){
   for(size_t i=0;i+1<a->shaderLog.size()&&text[i];++i)a->shaderLog[i]=text[i];
  }
 }
 bool WantsTail(uint64_t epoch){std::lock_guard<std::mutex> lock(mutex);auto* a=find(epoch);return a&&!a->terminal;}
 Snapshot Copy(){std::lock_guard<std::mutex> lock(mutex);return data;}
};

// All error getters execute before recording/cleanup on the calling thread.
template<class Call,class Good,class Error,class Stamp,class Emit>
auto ObserveCall(const char* enter,const char* exit,uint64_t tid,Call call,Good good,Error error,Stamp now,Emit emit){
 emit(Point{enter,now(),tid},false);
 auto value=call();
 const bool ok=good(value);
 const uint32_t raw=ok?0:uint32_t(error());
 Point row{exit,now(),tid};row.result=ok?1:0;row.rawEgl=raw;emit(row,!ok);
 return value;
}
inline void Number(std::ostream& o,const char* field,uint64_t n){o<<'"'<<field<<"\":\""<<n<<"\",";}
inline void JsonPoint(std::ostream& o,const Point& p){
 o<<"{\"stage\":\""<<p.stage<<"\",";Number(o,"ns",p.ns);Number(o,"tid",p.tid);Number(o,"root",p.root);
 Number(o,"a",p.a);Number(o,"b",p.b);Number(o,"c",p.c);
 o<<"\"result\":"<<p.result<<",\"rawEgl\":"<<p.rawEgl<<",\"rawGl\":"<<p.rawGl<<'}';
}
inline void JsonString(std::ostream& o,const char* text){
 o<<'"';for(const unsigned char* p=reinterpret_cast<const unsigned char*>(text);*p;++p){
  if(*p=='"'||*p=='\\')o<<'\\'<<char(*p);
  else if(*p>=32&&*p<127)o<<char(*p);
  else {const char* hex="0123456789abcdef";o<<"\\u00"<<hex[*p>>4]<<hex[*p&15];}
 }o<<'"';
}
inline void Json(std::ostream& o,const Snapshot& s,const char* profile,uint64_t pid){
 o<<"{\"revision\":\"T451_GPU_BOOT_PROBE_V1\",\"buildProfile\":\""<<profile<<"\",";
 Number(o,"pid",pid);
 const auto na=std::min<uint64_t>(s.attemptsSeen,s.attempts.size()),nt=std::min<uint64_t>(s.transitionsSeen,s.transitions.size());
 Number(o,"droppedAttempts",s.attemptsSeen-na);Number(o,"droppedTransitions",s.transitionsSeen-nt);
 o<<"\"attempts\":[";
 for(uint64_t i=0;i<na;++i){if(i)o<<',';const auto& a=s.attempts[(s.attemptsSeen-na+i)%s.attempts.size()];
  o<<'{';Number(o,"bootSerial",a.serial);Number(o,"epoch",a.epoch);Number(o,"surfaceGeneration",a.surface);Number(o,"windowId",a.window);
  Number(o,"candidateRun",a.candidateRun);Number(o,"publishedRun",a.publishedRun);Number(o,"readyEpoch",a.readyEpoch);
  Number(o,"session",a.session);Number(o,"ownerTid",a.ownerTid);
  o<<"\"actualClock\":\""<<(a.v2?"GRS_M1_PROGRESS_CLOCK_V2":"GRS_M1_PROGRESS_CLOCK_V1")<<"\",\"K\":"<<a.k<<",\"Kw\":"<<a.kw
   <<",\"width\":"<<a.width<<",\"height\":"<<a.height<<",\"terminal\":"<<(a.terminal?"true":"false")<<",\"complete\":"<<(a.terminal&&!a.dropped?"true":"false")<<',';
  Number(o,"dropped",a.dropped);o<<"\"shaderLog\":";JsonString(o,a.shaderLog.data());o<<',';o<<"\"firstFailure\":";JsonPoint(o,a.firstFailure);o<<",\"latest\":";JsonPoint(o,a.latest);o<<",\"rows\":[";
  for(size_t j=0;j<a.count;++j){if(j)o<<',';JsonPoint(o,a.points[j]);}o<<"]}";
 }
 o<<"],\"transitions\":[";
 for(uint64_t i=0;i<nt;++i){if(i)o<<',';const auto& t=s.transitions[(s.transitionsSeen-nt+i)%s.transitions.size()];
  o<<"{\"reason\":\""<<t.reason<<"\",";Number(o,"ns",t.ns);Number(o,"tid",t.tid);Number(o,"beforeEpoch",t.before);
  Number(o,"afterEpoch",t.after);Number(o,"surfaceGeneration",t.surface);Number(o,"a",t.a);Number(o,"b",t.b);o<<"\"result\":"<<t.result<<'}';
 }
 o<<"]}";
}
}
