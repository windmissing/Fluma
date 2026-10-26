#include "scene_gpu.h"
#include <algorithm>
namespace p013 {
Status SceneGlGpu::Begin(const ScenePacket& p){
 stage="SCENE_FULLFRAME_TARGETS";auto code=targets.Ensure(int(p.frame.surfaceWidthPx),int(p.frame.surfaceHeightPx),p.contextEpoch);if(code!=Status::OK)return code;
 stage="SCENE_LAYERS_SHADER_RESOURCE";code=layers.Ensure(p.contextEpoch);if(code!=Status::OK)return code;stage="SCENE_DATA_UPLOAD";code=layers.Upload(p.data.data());if(code!=Status::OK)return code;
 glBindFramebuffer(GL_FRAMEBUFFER,targets.fbos[targets.candidate]);glViewport(0,0,targets.width,targets.height);
 glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_SAMPLE_COVERAGE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDisable(GL_BLEND);
 return glGetError()==GL_NO_ERROR?Status::OK:Status::GL_RESOURCE_FAILED;
}
Status SceneGlGpu::Layer(int layer,const ScenePacket& packet,const SceneConfiguration& c,const SceneLease* lease){
 const auto& f=packet.frame;glBindFramebuffer(GL_FRAMEBUFFER,targets.fbos[targets.candidate]);glViewport(0,0,targets.width,targets.height);
 if(layer==0)glDisable(GL_BLEND);else{glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);}
 if(layer==2&&c.Material()){
  if(!lease||!lease->textures)return Status::NOT_READY;
  MaterialFrame frame;frame.frame=f;frame.frame.T_dataLayoutVersion=2;frame.frame.appearanceConfigRevision=c.revision;frame.assetGeneration=lease->desc->generation;frame.contextEpoch=packet.contextEpoch;frame.lightEnabled=c.values[0]==2&&c.values[2]!=8;
  auto params=c.Params();params.canvasToSurfaceScaleX=f.boxWidthPx/params.canvasBox[2];params.canvasToSurfaceScaleY=f.boxHeightPx/params.canvasBox[3];
  for(int i=0;i<3;i++){params.grainOffsetCanvasPx[i]=packet.motion[i];params.bandMinYCanvasPx[i]=packet.motion[i+3];params.bandMaxYCanvasPx[i]=packet.motion[i+6];}
  std::array<float,1536> check{};std::copy(packet.data.begin(),packet.data.begin()+1536,check.begin());
  if(ValidateMaterialFrame(frame,c.Light(),params,check)!=0||lease->desc->revision!=c.revision||lease->desc->lambdaMaterial!=params.effectiveLambdaMaterial||lease->desc->lambdaCanvas!=params.effectiveLambdaCanvasPx||lease->desc->warpRange!=params.warpRangeCycles||lease->desc->grainBase!=params.grainBaseSrgb||lease->desc->seed!=c.values[3]||lease->desc->geometryEpoch!=c.values[4])return Status::INVALID_ARGUMENT;
  if(material.epoch!=packet.contextEpoch){auto code=material.Create(sources,packet.contextEpoch);if(code!=Status::OK)return code;}
  return material.Draw(frame,c.Light(),params,packet.data.data(),*lease->textures);
 }
 if(layer==2||layer==3){
  DualVisual visual;visual.radiusPx=c.values[19];for(int i=0;i<6;i++){visual.colors[i]=c.values[29+i];visual.colors[i+6]=c.values[29+i];}
  auto frame=f;frame.T_dataLayoutVersion=1;
  return dual.DrawTarget(frame,layers.data,packet.xy.data(),packet.meta.data(),packet.count,packet.capacity,layer==3&&packet.particleEnabled,visual,packet.contextEpoch,targets.fbos[targets.candidate],false,layer==2);
 }
 return layers.Draw(layer,packet,c);
}
Status SceneGlGpu::Present(bool old){auto texture=targets.textures[old?targets.lastGood:targets.candidate];if(!texture||!layers.present)return Status::NOT_READY;return layers.Present(texture,targets.width,targets.height);}
}
