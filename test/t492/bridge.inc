 // T492 independent bounded observations; no diagnostic endpoint wakes owner.
 std::unique_ptr<TimelineSnapshot> stableTimeline{new TimelineSnapshot};
 diagnostic492::Capture diagnostics;
 diagnostic492::Scope DiagnosticScopeLocked()const{
  auto value=[](U64 x){return (uint64_t(x.hi)<<32)|x.lo;};
  return {epoch.load(),value(identity.run),value(identity.context),value(identity.surface),progress.session,recovery.serial,permissionRevision};
 }
 diagnostic492::Point DiagnosticPointLocked(const char* name,uint64_t root=0,uint64_t a=0,uint64_t b=0,uint64_t c=0,uint32_t code=0,uint32_t gl=0,uint32_t egl=0){
  diagnostic492::Point p;p.scope=DiagnosticScopeLocked();p.ns=Stamp();p.tid=BridgeTid();p.root=root;p.a=a;p.b=b;p.c=c;p.code=code;p.gl=gl;p.egl=egl;
  std::strncpy(p.stage.data(),name,p.stage.size()-1);return p;
 }
 void DiagnosticLocked(const char* stage,uint64_t root=0,uint64_t a=0,uint64_t b=0,uint64_t c=0,uint32_t code=0,uint32_t gl=0,uint32_t egl=0){diagnostics.Add(DiagnosticPointLocked(stage,root,a,b,c,code,gl,egl));}
 void DiagnosticFaultLocked(const char* reason){
  if(!progress.open&&!jobLive&&!AllowedLocked())return;
  const auto p=DiagnosticPointLocked("FIRST_REVOKE",permit.nonce,permitStarted,permit.deadline,progress.receiveNs);
  const char* original=std::strcmp(reason,"PAUSED_CLOCK")==0?progress.fault:reason;
  diagnostics.Fault(p,original,DiagnosticRootLocked());diagnostics.Add(p);
 }
 diagnostic492::Root DiagnosticRootLocked(){
  diagnostic492::Root root;root.scope=DiagnosticScopeLocked();root.nonce=permit.nonce;root.started=permitStarted;root.deadline=permit.deadline;root.kind=permitKind;root.plan=permitPlan;root.identity=permit.identity;root.input=permit.input;
  return root;
 }
 void DiagnosticIntentLocked(){
  diagnostics.Intent(DiagnosticRootLocked());DiagnosticLocked("ROOT_INTENT",permit.nonce,permitStarted,permit.deadline,permitPlan);
 }
 void DiagnosticActionLocked(const std::string& raw){diagnostics.ActionRecord(DiagnosticPointLocked("UI_ACTION"),raw);}
 void DiagnosticOfferLocked(const std::array<uint32_t,32>& words,const char* result){
  diagnostics.PacketRecord(DiagnosticPointLocked("NATIVE_OFFER",permit.nonce,words[6],words[2],words[24],std::strcmp(result,"ENQUEUED_NOT_ACK")?1:0),words,result);
 }
 SnapshotScope SnapshotScopeLocked()const{
  SnapshotScope s;const auto d=DiagnosticScopeLocked();s.epoch=d.epoch;s.run=d.run;s.context=d.context;s.surface=d.surface;
 #if defined(_WIN32)
  s.pid=uint64_t(_getpid());
 #else
  s.pid=uint64_t(getpid());
 #endif
  // A source declaration is not an installed-HAP attestation. T483 remains authoritative.
  const auto package=programcache::packageSource;
  if(package.size()<=128&&package.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.")==std::string::npos)std::memcpy(s.package.data(),package.data(),package.size());
  std::strncpy(s.route.data(),kT492Route,s.route.size()-1);std::strncpy(s.source.data(),kT492Source,s.source.size()-1);return s;
 }
 const char* SnapshotBeginLocked(TimelineSnapshot::Page& page){
  auto result=stableTimeline->Begin(timeline,SnapshotScopeLocked(),Stamp());
  return std::strcmp(result,"OK")==0?stableTimeline->Read(stableTimeline->id,0,Stamp(),page):result;
 }
