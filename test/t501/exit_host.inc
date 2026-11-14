#include <fstream>
static unsigned mode501=0,phase501=0;static bool sawFailure501=false;static uint64_t creates501=0,start501=0;
static void ExitWake(App& a,BridgeStage stage){
 if(stage!=BridgeStage::Wake)return;std::lock_guard<std::mutex> lock(a.mutex);assert(programcache::Stamp()-start501<600000000000ULL);
 const auto state=a.resources->Copy();
 if(mode501>=5&&phase501==1&&!state.exit.complete&&state.exit.firstError){sawFailure501=true;assert(state.creates==creates501);if(mode501!=8)assert(state.retiringBytes>0);assert(state.exit.display&&state.exit.window);}
 if(a.recovery.phase!=recovery467::Phase::WAIT_NEW_SESSION)return;
 if(mode501>=5&&phase501==0){phase501=1;creates501=state.creates;failureStage=mode501-4;failureRemaining=1;a.InvalidateLocked();a.RecoveryRequestLocked(true);return;}
 if(mode501>=5){assert(sawFailure501&&state.exit.complete&&state.exit.firstError&&state.retiringBytes==0);for(const auto& r:state.objects)if(r.state==resource501::State::Live)assert(r.scope==a.epoch.load());}
 if(mode501>0&&mode501<5){failureStage=mode501;failureRemaining=1;}
 a.quit=true;
}
int main(int argc,char** argv){
 assert(argc==4);mode501=unsigned(std::stoi(argv[2]));assert(programcache::Store().Configure(argv[1]));start501=programcache::Stamp();
 auto app=std::make_unique<App>(false);app->window=reinterpret_cast<void*>(1);app->hostHook=ExitWake;app->nextProgressV2=true;app->invalidate(nullptr,false);app->loop();
 auto state=app->resources->Copy();assert(state.exit.attempted&&state.exit.shutdown);
 if(mode501>0&&mode501<5){assert(!state.exit.complete&&state.exit.firstError&&state.exit.display&&state.exit.window);if(mode501<4)assert(state.retiringBytes>0);}
 else assert(state.exit.complete&&state.liveBytes==0&&state.retiringBytes==0);
 std::ofstream(argv[3])<<state.Json();programcache::Store().Drain();
 std::cout<<"{\"revision\":\"T501_EGL_EXIT_V1\",\"mode\":"<<mode501<<",\"passed\":true,\"device\":\"NOT_RUN\"}"<<std::endl;
}
