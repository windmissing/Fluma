#include "boot_probe.h"
#include <cassert>
#include <iostream>
#include <string>
using namespace grs::boot456;
int main(){
 Probe p;
 p.Begin(1,0,false,12,192,128,false,1);
 p.Mark(1,{"WAIT_WINDOW",2,12});
 assert(p.Copy().attempts[0].window==0);
 p.TransitionAt({"SURFACE_CREATED",3,4,1,2,1,192,128},true);
 p.Begin(2,1,true,12,192,128,false,4);
 auto attempt=p.Copy();assert(attempt.attempts[0].terminal&&attempt.attempts[0].firstFailure.stage==std::string("SURFACE_CREATED"));
 uint64_t t=10;bool called=false,gotError=false;
 const auto bad=ObserveCall("EGL_ENTER","EGL_EXIT",12,[&]{called=true;return 0;},[](int v){return v==1;},[&]{assert(called);gotError=true;return 0x3003;},[&]{return ++t;},[&](const Point& row,bool fail){if(fail)assert(gotError);p.Mark(2,row,fail,fail);});
 assert(!bad&&p.Copy().attempts[1].firstFailure.rawEgl==0x3003);
 p.ShaderLog(2,"line \"foo\"\nnext");
 p.Mark(2,{"AFTER_CLEANUP",++t,12,0,0,0,0,0,0x3000},true);
 p.TransitionAt({"UI_RESET",++t,4,2,3,1},true);
 p.Begin(3,1,true,12,192,128,true,++t);
 p.Mark(2,{"LATE_FAILURE",++t,12},true);
 assert(!p.Copy().attempts[2].firstFailure.stage[0]);
 p.Identity(3,4,3,2,6,192,128);
 p.Intent(3,1,{"ROOT_INTENT",++t,12,1});p.Root(3,{"ROOT_ACCEPT",++t,12,1});
 p.Draw(3,{"CURRENT",++t,12,1},false);assert(p.WantsTail(3));
 p.Intent(3,2,{"ROOT_INTENT",++t,12,2});p.Root(3,{"DISPATCH_ENTER",++t,12,2});
 assert(p.Copy().attempts[2].latest.stage==std::string("DISPATCH_ENTER"));
 p.Root(3,{"ROOT_ACCEPT",++t,12,2});assert(!p.WantsTail(3));
 for(unsigned i=0;i<300;++i)p.Mark(3,{"TEST_OVERFLOW",++t,12});
 assert(p.Copy().attempts[2].dropped>0);
 Json(std::cout,p.Copy(),"T456_EXPERIMENT",1);std::cout<<'\n';
}
