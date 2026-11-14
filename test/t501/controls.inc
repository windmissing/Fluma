int main(int argc,char** argv){assert(PreviousControls(argc,argv)==0);unsigned cases=0;
 resource501::Ledger ledger;ledger.Create(1,1,resource501::Kind::Buffer,7,0);ledger.Allocate(1,1,resource501::Kind::Buffer,7,1024,0);
 ledger.Allocate(1,1,resource501::Kind::Buffer,7,2048,0);auto snap=ledger.Copy();assert(snap.liveBytes==2048&&snap.logicalPeak==2048&&snap.replaces==1);++cases;
 ledger.Allocate(1,1,resource501::Kind::Buffer,7,4096,GL_OUT_OF_MEMORY);snap=ledger.Copy();assert(snap.liveBytes==2048&&snap.allocFailures==1);++cases;
 ledger.Tombstone(1,1);snap=ledger.Copy();assert(snap.liveBytes==0&&snap.retiringBytes==2048);ledger.Create(2,2,resource501::Kind::Buffer,7,0);ledger.Allocate(2,2,resource501::Kind::Buffer,7,100,0);
 ledger.Retired(1,1);snap=ledger.Copy();assert(snap.liveBytes==100&&snap.retiringBytes==0);++cases;
 ledger.Delete(2,2,resource501::Kind::Buffer,7,GL_INVALID_OPERATION);assert(ledger.Copy().liveBytes==100);ledger.Delete(2,2,resource501::Kind::Buffer,7,0);assert(ledger.Copy().liveBytes==0);++cases;
 ledger.ExitStage("DESTROY_CONTEXT_FAILED",2,true,1,2,3,4,EGL_BAD_CONTEXT);ledger.ExitStage("TERMINATE_FAILED",2,true,1,0,0,4,EGL_BAD_DISPLAY);snap=ledger.Copy();assert(!snap.exit.complete&&snap.exit.firstError==EGL_BAD_CONTEXT&&snap.exit.error==EGL_BAD_DISPLAY&&snap.exit.window==4);++cases;
 ledger.HostStorage("CAPTURE",42,100,128);ledger.HostStorage("CAPTURE",43,100,128);assert(ledger.Copy().hostPeak==256);ledger.HostStorage("CAPTURE",42,0,0,false);assert(ledger.Copy().hostPeak==256);++cases;
 for(unsigned i=0;i<1000;++i){ledger.Create(2,2,resource501::Kind::Buffer,7,0);ledger.Delete(2,2,resource501::Kind::Buffer,7,0);}assert(ledger.Copy().incomplete==0&&ledger.Copy().dropped>0);++cases;
 std::ofstream(std::string(argv[1])+"/ledger.json")<<ledger.Copy().Json();
 std::cout<<"{\"revision\":\"T501_LEDGER_CONTROLS_V1\",\"passed\":"<<cases<<",\"failed\":0}"<<std::endl;
}
