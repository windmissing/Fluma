#pragma once
#include "single_sand_packet.h"
#include "fullframe_targets.h"
#include "egl_context.h"
namespace p013 {
struct SingleSandAsset {
 SingleSandConfig config;
 std::array<uint8_t,65536> bytes{};
 uint64_t generation=0;
 GLuint texture=0;
};
struct SingleSandReceipt {
 std::string reason="SINGLE_DRAW_FAILED",state="FAULT",actual,actualDigest,actualGrain,actualProfile,actualPb;
 const char* stage="VALIDATE";
 uint32_t rawGl=0;
 int rawEgl=0,encoding=0,colorSpace=0,samples=0,highp=0;
 bool swapped=false,replayed=false,writeControl=false,writeEnabled=false;
 const char* bedSupport="NOT_EVALUATED";
 const char* lease="NOT_ACQUIRED";
 size_t uploaded=0,targets=0;
 uint32_t textureCount=0,drawCalls=0,occludedCenters=0;
 double wallMs=0;
};
class SingleSandPass {
 public:
 SingleSandConfig pendingConfig;
 SingleSandAsset active,pending;
 uint64_t lastGeneration=0;
 const char* Stage(uint64_t,const std::string&,const uint8_t*);
 SingleSandReceipt Draw(EglContext&,const SingleSandPacket&);
 void Reset(bool);
 GLuint program=0;
#ifdef P013_SINGLE_SAND_HOST_TEST
 GLuint TestCommittedFramebuffer()const{return targets.fbos[targets.lastGood];}
#endif
 private:
 FullFrameTargets targets;
 GLuint vao=0,profile=0,xy=0,meta=0;
 uint64_t epoch=0,shownSurface=0,shownFrame=0;
 std::string shownToken,shownDigest,shownGrain,shownProfile,shownPb;
 bool Ensure(uint64_t);
};
const char* SingleSandShaderHash();
}
