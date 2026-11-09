#include "../../entry/src/main/cpp/gpu_resident/evidence_capture.h"
#include <cassert>
#include <fstream>
#include <filesystem>
#include <iostream>
using namespace grs;
int main(){
 RenderCadence30 cadence;unsigned draws=0;
 for(uint64_t i=0;i<1200;i++)if(cadence.Due(100+i*1000000000/120,true))++draws;
 assert(draws>=299&&draws<=301&&cadence.skipped+draws==1200);
 assert(cadence.Due(1,false));
 cadence.Stale(1);cadence.Presented(11,1,2);cadence.Presented(21,1,2);cadence.Presented(31,2,1);
 assert(cadence.newPhysicalFrames==1&&cadence.sourceOnlyFrames==1&&cadence.staleNs==10);
 EvidenceConfig config;config.packageSha256=std::string(64,'a');config.expectedRevision="GRS_M1_PROGRESS_CLOCK_V2";
 config.epoch=config.pageSession=config.runLo=1;config.durationNs=2000000000;assert(config.Valid());
 auto c=std::make_unique<EvidenceCapture>();EvidenceLedger ledger;EvidenceControl ctl;BridgeTimeRow row;
 row.ns=row.now=1;row.tid=17;row.root=1;c->evidenceSource="SYNTHETIC_PROTOCOL";c->clockDomain="TEST_SEQUENCE_NS";c->uiDomain="TEST_CALLBACK_NS";c->Begin(config,1,1,ledger);
 c->Push("BEGIN","",row,ledger,ctl,true);c->readySeen=true;c->Push("READY","",row,ledger,ctl,true);
 // First detailed root is fixed. All later roots are explicit aggregates.
 row.stage=63;c->Detail(row,ledger,ctl);row.stage=43;c->Detail(row,ledger,ctl);
 auto before=c->count;row.root=2;for(unsigned i=0;i<10000;i++)c->Detail(row,ledger,ctl);
 assert(c->count==before&&c->suppressedDetail>=10000&&c->dropped==0);
 c->Pulse(1,10,10,true);c->Pulse(2,16000010,16000010,true);c->Pulse(2,16000010,16000010,false);
 assert(c->pulses==2&&c->rejectedPulses==1&&c->maxNativeGap==16000000);
 ledger.generated=ledger.accepted=1;c->acceptedSeen=true;c->Push("FIRST_ACCEPT","",row,ledger,ctl,true);
 assert(c->SaveRoot(0,1,1,0,0,1,2,2,"GRS_ACCEPTED_ROOT_Q20_M1_V1|1"));
 c->Present(1,"scene-token","scene-token",true,true);
 c->Freeze("EXPLICIT_END",100,ledger,ctl);assert(c->frozen&&!c->incomplete);
 const auto stable=c->State();c->Freeze("OTHER",200,ledger,ctl);assert(stable==c->State());
 std::filesystem::create_directories(".codex/local-test/t409-pages");
 size_t bytes=0;for(uint32_t i=0;i<c->PageCount();i++){const auto page=c->Page(i);assert(page.size()<16384);bytes+=page.size();std::ofstream(".codex/local-test/t409-pages/page"+std::to_string(i)+".json")<<page;}
 assert(bytes<config.maxBytes);
 auto full=std::make_unique<EvidenceCapture>();config.maxRows=16;full->Begin(config,2,1,ledger);
 full->Push("BEGIN","",row,ledger,ctl,true);row.root=1;row.stage=63;full->Detail(row,ledger,ctl);row.stage=43;full->Detail(row,ledger,ctl);
 assert(full->frozen&&full->incomplete&&full->dropped==1&&std::string(full->rows[full->count-1].kind.data())=="END");
 config.durationNs=1999999999;assert(!config.Valid());config.durationNs=5000000001;assert(!config.Valid());
 std::cout<<"T409 native capture fixed budget/frozen pages/idempotent end/explicit aggregates and 30Hz 120Hz independence PASS draws="<<draws<<" bytes="<<bytes<<" storage="<<sizeof(EvidenceCapture)<<"\n";
}
