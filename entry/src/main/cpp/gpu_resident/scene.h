#pragma once
#include "source_transport.h"
#include "../p013renderer/fullframe_targets.h"
#include <string>
namespace grs {
struct ResidentSceneFrame {U64 run,context,surface,geometry;uint64_t renderFrame=0;int width=0,height=0;float boxX=0,boxY=0,boxWidth=0,boxHeight=0,dpi=1;bool mirror=false;};
struct ResidentSceneReceipt {bool current=false,swapSucceeded=false,displayPresentVerified=false;uint32_t rawGl=0,rawEgl=0;const char* stage="NOT_READY";std::string requestedToken,renderedToken;uint64_t frameTargetBytes=0;};
class ResidentScene {
 public:ResidentScene()=default;ResidentScene(const ResidentScene&)=delete;ResidentScene& operator=(const ResidentScene&)=delete;
 using Swap=bool(*)(void*);
 ResidentSceneReceipt Draw(SourceGpuTransport&,uint32_t slot,U64 publication,const ResidentSceneFrame&,Swap,void*);
 void Reset(bool current);
 const std::string& LastGoodToken()const{return lastGood;}
 private:GLuint full=0,particles=0,vao=0,particleVao=0,bed=0;EGLContext context=EGL_NO_CONTEXT;p013::FullFrameTargets targets;std::string lastGood;uint64_t lastRenderFrame=0;
 bool Ensure();void Geometry(GLuint,const ResidentSceneFrame&);std::string Token(const PublicationLease&,const ResidentSceneFrame&);
};
}
