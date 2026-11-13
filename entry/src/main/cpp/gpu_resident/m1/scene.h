#pragma once
#include "full_transport.h"
#include "../scene.h"
#include "../program_cache/graphics_program_factory.h"
namespace grs {namespace m1 {
struct SceneOptics {std::array<float,16> values{{.03f,.04f,.13f,.18f,.6f,1,1,0,1,.8f,.9f,1,.02f,16,.6f,.8f}};float waterRadiusPx=2,massReference=.125f;};
std::array<programcache::GraphicsProgramDesc,3> ScenePrograms();
class Scene {
public:
 using TokenReady=void(*)(const std::string&,void*);
 using Capture=void(*)(uint32_t,GLuint,void*); // explicit cold pixel test seam; default null
 using CandidateCheck=bool(*)(void*);
 bool PreparePrograms(uint64_t contextGeneration,CandidateCheck=nullptr,void* user=nullptr,bool cacheEnabled=true);
 const std::string& PrepareFailure()const{return prepareFailure;}
 uint64_t PrepareWallNs()const{return prepareWallNs;}
 ResidentSceneReceipt Draw(FullM1Transport&,uint32_t,U64,const ResidentSceneFrame&,ResidentScene::Swap,void*,const SceneOptics& cold=SceneOptics{},Capture=nullptr,TokenReady=nullptr);
 void Reset(bool);const std::string& LastGoodToken()const{return lastGood;}
private:
 GLuint full=0,particles=0,water=0,vao=0,particleVao=0;EGLContext context=EGL_NO_CONTEXT;p013::FullFrameTargets targets;std::string lastGood;uint64_t lastRenderFrame=0;
 bool Ensure(uint64_t);void Geometry(GLuint,const ResidentSceneFrame&);std::string Token(const PublicationLease&,const ResidentSceneFrame&,const SceneOptics&);
 std::array<programcache::GraphicsUniformTable,3> uniforms{};
 uint64_t contextGeneration=0,prepareWallNs=0;std::string prepareFailure;
 SceneOptics boundOptics{};bool opticsBound=false;
};
}}
