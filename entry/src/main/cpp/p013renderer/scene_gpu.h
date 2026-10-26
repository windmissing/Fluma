#pragma once
#include "scene_compositor.h"
#include "scene_layers.h"
#include "fullframe_targets.h"
#include "dualfield_pass.h"
#include "egl_context.h"
namespace p013 {
class SceneGlGpu final:public SceneGpu {
 public:
 SceneGlGpu(EglContext& e,MaterialPass& m,DualFieldPass& d,ShaderSources shaders):egl(e),material(m),dual(d),sources(shaders){}
 const char* FailureStage()const override{return stage;}
 Status Begin(const ScenePacket&)override;
 Status Layer(int,const ScenePacket&,const SceneConfiguration&,const SceneLease*)override;
 Status Present(bool)override;
 Status Swap(uint64_t id)override{return egl.Swap(id);}
 void SetBaseStripTexture(uint32_t texture)override{baseStripTexture=texture;}
 void CommitTargets() noexcept override {targets.Commit();}
 void Reset(bool current){targets.Reset(current);layers.Reset(current);}
 FullFrameTargets targets;SceneLayers layers;
 private:const char* stage="SCENE_TARGETS";EglContext& egl;MaterialPass& material;DualFieldPass& dual;ShaderSources sources;
 uint32_t baseStripTexture=0;
};
}
