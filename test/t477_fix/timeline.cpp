#include "bridge_timeline.h"
#include <cassert>
#include <fstream>
#include <memory>
int main(int argc,char** argv) {
 assert(argc==2);
 static_assert(uint32_t(grs::TailStage::BatchFlushEnter)==66);
 static_assert(uint32_t(grs::TailStage::BatchFlushExit)==67);
 auto timeline=std::make_unique<grs::BridgeTimeline>(true,true,true);
 for(unsigned i=0;i<124;i++) {
  grs::BridgeTimeRow row;
  row.ns=row.now=1000+i;row.tid=row.epoch=row.run=row.surface=row.input=1;
  row.stage=i<120?6:i==120?66:i==121?67:i==122?13:16;
  if(i>=120) {
   row.root=1;row.probeStart=1100;
   row.probeEnd=i==120?0:1121;
  }
  timeline->Add(row);
 }
 std::ofstream output(argv[1]);
 for(unsigned cursor=0;cursor<124;cursor+=4)output<<timeline->Export(cursor)<<'\n';
 assert(output.good());
}
