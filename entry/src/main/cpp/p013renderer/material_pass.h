// T55 G3 isolated complete sand pass. No page, Canvas, or physics dependency.
#pragma once
#include <GLES3/gl3.h>
#include "render_contract.h"
#include "shader_pass.h"
namespace p013 {
struct MaterialParams {
 std::array<double,6> paletteBaseSrgb{};
 std::array<double,9> grainBaseSrgb{};
 std::array<double,3> grainOffsetCanvasPx{},bandMinYCanvasPx{},bandMaxYCanvasPx{};
 std::array<double,4> canvasBox{};
 double canvasToSurfaceScaleX=0,canvasToSurfaceScaleY=0;
 double aB=.75,aC=.5,aC3=.10,warpRangeCycles=0,effectiveLambdaMaterial=0,effectiveLambdaCanvasPx=0;
 bool grainEnabled=true,strataEnabled=true,sparkEnabled=true;
};
struct MaterialFrame {
 FrameInput frame;
 uint64_t assetGeneration=0,contextEpoch=0;
 bool lightEnabled=false;
};
int ValidateMaterialFrame(const MaterialFrame&,const LightParams&,const MaterialParams&,const std::array<float,1536>&);
class MaterialPass {
 public:
 Status Create(const ShaderSources&,uint64_t contextEpoch);
 Status Draw(const MaterialFrame&,const LightParams&,const MaterialParams&,const float*,const std::array<uint32_t,2>& textures,uint32_t baseStrip=0,int viewportOriginX=0);
 void Reset(bool contextCurrent);
 GLuint Program()const{return program_;} GLuint Vao()const{return vao_;} GLuint DataTexture()const{return dataTexture_;}
 uint64_t epoch=0,uploadCount=0,textureAllocCount=0;
 GLenum error=0;const char* errorStage="NONE";
 double uploadMs=0,drawMs=0;
 private:
 GLuint Compile(GLenum,const char*);
 GLuint program_=0,vao_=0,dataTexture_=0;
 std::array<GLint,25> uniforms_{};
};
}
