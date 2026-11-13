#include <fstream>
int main(int argc,char** argv){
 assert(argc==2);assert(OriginalControls()==0);const std::string out=argv[1];unsigned cases=0;
 auto a=std::make_unique<App>(false);a->hostNow=[](){return nowNs;};a->window=reinterpret_cast<void*>(1);a->ui.SetMode(false);
 Request(*a,true);Resources(*a,101);auto session=Handoff(*a);a->PagePulseLocked(session,1,++nowNs);
 a->permit={a->identity,a->input,a->epoch,a->permissionRevision,nowNs+250000000,7,a->window};a->permitSession=session;a->permitPlan=1;a->progress.active.id=1;a->permitKind=2;a->permitStarted=nowNs;a->jobLive=true;a->DiagnosticIntentLocked();
 a->accepted=1;a->physicsCommit=7;
 a->RevokeLocked("GPU_PROGRESS_TIMEOUT");a->phase="GPU_PROGRESS_TIMEOUT";
 const auto revision=a->permissionRevision;const auto reason=std::string(a->recovery.reason);auto* group=a->diagnostics.Find(a->DiagnosticScopeLocked());
 assert(group&&group->cause.set&&std::string(group->cause.reason.data())=="GPU_PROGRESS_TIMEOUT");const auto first=group->cause.point;++cases;
 for(unsigned i=0;i<100;++i){nowNs+=1000000;assert(std::string(a->PagePulseLocked(session,2+i,nowNs))=="STALE_PAGE_SESSION");}
 assert(a->permissionRevision==revision&&std::string(a->recovery.reason)==reason&&group->cause.point.ns==first.ns&&a->phase==std::string("GPU_PROGRESS_TIMEOUT"));assert(group->cause.latePulses==100&&a->accepted==1&&a->physicsCommit==7);++cases;
 // A post-terminal Swap check remains visible, never changes first cause or grants.
 a->DiagnosticLocked("SWAP_CHECK_ENTER",7);RootRequest root{};root.kind=2;root.nonce={7,0};assert(!a->PermitLocked(root,nowNs));a->DiagnosticLocked("SWAP_CHECK_EXIT",7,0,0,0,1);
 assert(group->tailCount>0&&group->cause.point.ns==first.ns);++cases;
 Request(*a,true);Resources(*a,102);auto next=Handoff(*a);a->PagePulseLocked(next,1,++nowNs);
 const auto before=a->progress.generated;assert(std::string(a->PagePulseLocked(session,1000,++nowNs))=="STALE_PAGE_SESSION");assert(a->AllowedLocked()&&a->progress.generated==before);++cases;
 nowNs+=33333334;assert(std::string(a->PagePulseLocked(next,2,nowNs))=="CLOCK_GAP");assert(!a->AllowedLocked()&&!a->progress.open);++cases;
 auto* gap=a->diagnostics.Find(a->DiagnosticScopeLocked());assert(gap&&std::string(gap->cause.reason.data())=="CLOCK_GAP");++cases;
 // Snapshot copied actual native rows and token table, while live writes continue.
 a->timeline.count=0;a->timeline.serial=0;a->timeline.dropped=0;a->timeline.tokenCount=0;
 auto row=[&](uint32_t stage){BridgeTimeRow r;r.ns=r.now=++nowNs;r.tid=BridgeTid();r.epoch=a->epoch;r.run=102;r.stage=stage;r.probeStart=nowNs;r.probeEnd=stage==66?0:nowNs;return r;};
 for(unsigned i=0;i<124;++i)a->timeline.Add(row(i==120?66:i==121?67:1));
 const auto token=a->timeline.SaveTokens("requested-token","rendered-token");auto tokenRow=row(50);tokenRow.receipt=token;a->timeline.Add(tokenRow);
 TimelineSnapshot::Page p;assert(std::string(a->SnapshotBeginLocked(p))=="OK");const auto id=p.id,scopeEpoch=p.scope.epoch;const auto frozenSerial=p.serial;
 const auto readyBefore=a->AllowedLocked();const auto epochBefore=a->epoch.load();const auto permBefore=a->permissionRevision;
 assert(std::string(a->SnapshotBeginLocked(p))=="SNAPSHOT_BUSY");assert(a->epoch==epochBefore&&a->permissionRevision==permBefore&&a->AllowedLocked()==readyBefore);++cases;
 for(unsigned i=0;i<500;++i)a->timeline.Add(row(1));
 for(uint32_t cursor=0;cursor<125;cursor+=4){assert(std::string(a->stableTimeline->Read(id,cursor,nowNs,p))=="OK");assert(p.serial==frozenSerial&&p.count==125&&p.scope.epoch==scopeEpoch);std::ofstream(out+"/page-"+std::to_string(cursor)+".json")<<p.Json();}
 assert(a->timeline.count==625);++cases;
 assert(std::string(a->stableTimeline->Read(id,124,nowNs,p))=="OK");assert(p.Json().find("requested-token")!=std::string::npos);const auto repeat=p.Json();a->stableTimeline->Read(id,124,nowNs,p);assert(p.Json()==repeat);++cases;
 assert(std::string(a->stableTimeline->Read(id,126,nowNs,p))=="INVALID_CURSOR");assert(std::string(a->stableTimeline->Read(id+1,0,nowNs,p))=="STALE_SNAPSHOT");++cases;
 Request(*a,true);assert(std::string(a->stableTimeline->Read(id,0,nowNs,p))=="OK"&&p.scope.epoch==scopeEpoch);++cases;
 assert(std::string(a->stableTimeline->End(id,nowNs))=="SNAPSHOT_RELEASED");assert(std::string(a->stableTimeline->Read(id,0,nowNs,p))=="STALE_SNAPSHOT");++cases;
 assert(std::string(a->SnapshotBeginLocked(p))=="OK"&&p.id>id);auto id2=p.id;
 assert(std::string(a->stableTimeline->Read(id2,0,p.capturedNs+60000000000ULL,p))=="STALE_SNAPSHOT");assert(std::string(a->stableTimeline->End(id2,nowNs))=="STALE_SNAPSHOT");++cases;
 a->timeline.dropped=77;assert(std::string(a->SnapshotBeginLocked(p))=="OK"&&p.dropped==77);a->stableTimeline->End(p.id,nowNs);++cases;
 a->stableTimeline->sequence=UINT64_MAX;assert(std::string(a->SnapshotBeginLocked(p))=="SNAPSHOT_ID_EXHAUSTED");++cases;
 // Summary count continues after legacy fixed prefix drops; unknowns remain unknown.
 for(unsigned i=0;i<10000;++i)a->DiagnosticLocked("SUMMARY_ONLY",0,i);
 bool found=false;for(unsigned i=0;i<a->diagnostics.stageCount;++i)if(std::string(a->diagnostics.stages[i].first.stage.data())=="SUMMARY_ONLY"){found=true;assert(a->diagnostics.stages[i].count==10000);}assert(found);++cases;
 std::array<uint32_t,32> words{};words[6]=123;a->DiagnosticOfferLocked(words,"NOT_READY");assert(a->diagnostics.packetCount==1&&a->diagnostics.packets[0].words==words);++cases;
 a->invalidate(nullptr,false,"UI_RESET",0,0,51);a->invalidate(nullptr,false,"UI_RESET",0,0,52);
 bool firstReset=false,secondReset=false;for(unsigned i=0;i<a->diagnostics.resetCount;++i){const auto& r=a->diagnostics.resets[i];if(std::string(r.stage.data())=="RESET_NATIVE_RECEIPT"){if(r.a==51)firstReset=true;if(r.a==52)secondReset=true;assert(r.c==r.b+1);}}assert(firstReset&&secondReset);++cases;
 for(unsigned i=1;i<=23;++i)a->MarkLocked(BridgeStage(i));bool rootPoll=false;for(unsigned i=0;i<a->diagnostics.stageCount;++i)if(std::string(a->diagnostics.stages[i].first.stage.data())=="ROOT_POLL")rootPoll=true;assert(rootPoll);++cases;
 auto capture=std::make_unique<diagnostic492::Capture>();diagnostic492::Root source;source.scope.epoch=99;source.nonce=1;source.kind=1;capture->Intent(source);auto quantum=source;quantum.nonce=2;quantum.kind=2;capture->Intent(quantum);
 diagnostic492::Point point;point.scope=source.scope;point.root=2;capture->Add(point);assert(capture->groups[0].quantumCount==1&&capture->groups[0].root.nonce==1&&capture->groups[0].quantum.nonce==2);
 auto later=quantum;later.nonce=3;point.root=3;capture->Fault(point,"LATER_ROOT_ERROR",later);capture->Add(point);assert(capture->groups[0].tailCount==1&&capture->groups[0].faultRoot.nonce==3);++cases;
 std::ofstream diagnostic(out+"/execution.json");a->diagnostics.Json(diagnostic);diagnostic.close();
 std::cout<<"{\"revision\":\"T492_NATIVE_CONTROLS_V1\",\"passed\":"<<cases<<",\"failed\":0,\"device\":\"NOT_RUN\"}"<<std::endl;
}
