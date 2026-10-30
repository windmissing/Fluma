#pragma once
#include "full_transport.h"
#include "../scene.h"
namespace grs {namespace m1 {
struct SceneOptics {std::array<float,16> values{{.03f,.04f,.13f,.18f,.6f,1,1,0,1,.8f,.9f,1,.02f,16,.6f,.8f}};float waterRadiusPx=2,massReference=.125f;};
class Scene {
public:
 using Capture=void(*)(uint32_t,GLuint,void*); // explicit cold pixel test seam; default null
 ResidentSceneReceipt Draw(FullM1Transport&,uint32_t,U64,const ResidentSceneFrame&,ResidentScene::Swap,void*,const SceneOptics& cold=SceneOptics{},Capture=nullptr);
 void Reset(bool);const std::string& LastGoodToken()const{return lastGood;}
private:
 GLuint full=0,particles=0,water=0,vao=0,particleVao=0;EGLContext context=EGL_NO_CONTEXT;p013::FullFrameTargets targets;std::string lastGood;uint64_t lastRenderFrame=0;
 bool Ensure();void Geometry(GLuint,const ResidentSceneFrame&);std::string Token(const PublicationLease&,const ResidentSceneFrame&,const SceneOptics&);
 SceneOptics boundOptics{};bool opticsBound=false;
};
}}
