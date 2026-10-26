#include "material_pass.h"
#include <algorithm>
namespace p013 {
int ValidateMaterialFrame(const MaterialFrame& material,const LightParams& light,const MaterialParams& p,const std::array<float,1536>& data){
 FrameInput legacy=material.frame;legacy.T_dataLayoutVersion=1;
 std::array<float,512> row{};std::copy(data.begin(),data.begin()+512,row.begin());
 if(Validate(legacy,light,row)!=Status::OK)return 400;
 if(material.frame.T_dataLayoutVersion!=2 || material.frame.zOriginVersion!=1 || !material.assetGeneration || !material.contextEpoch)return 400;
 for(float x:data)if(!std::isfinite(x))return 400;
 for(size_t c=0;c<128;c++)if(data[(128+c)*4+1]<0||data[(128+c)*4+1]>1||data[(256+c)*4+1]<0||data[(256+c)*4+2]!=0||data[(256+c)*4+3]!=0)return 400;
 for(double x:p.paletteBaseSrgb)if(!std::isfinite(x)||x<0||x>1)return 400;
 for(double x:p.grainBaseSrgb)if(!std::isfinite(x)||x<0||x>1)return 400;
 for(size_t i=0;i<3;i++)if(!std::isfinite(p.grainOffsetCanvasPx[i])||!std::isfinite(static_cast<float>(p.grainOffsetCanvasPx[i]))||
 !std::isfinite(p.bandMinYCanvasPx[i])||!std::isfinite(p.bandMaxYCanvasPx[i])||p.bandMaxYCanvasPx[i]<p.bandMinYCanvasPx[i])return 400;
 for(double x:p.canvasBox)if(!std::isfinite(x)||!std::isfinite(static_cast<float>(x)))return 400;
 if(p.canvasBox[2]<=0||p.canvasBox[3]<=0||!std::isfinite(p.canvasToSurfaceScaleX)||!std::isfinite(p.canvasToSurfaceScaleY)||p.canvasToSurfaceScaleX<=0||
 p.canvasToSurfaceScaleX!=p.canvasToSurfaceScaleY||material.frame.boxWidthPx/p.canvasBox[2]!=p.canvasToSurfaceScaleX||material.frame.boxHeightPx/p.canvasBox[3]!=p.canvasToSurfaceScaleY)return 400;
 if(p.aB!=.75||p.aC!=.5||p.aC3!=.10||!std::isfinite(p.warpRangeCycles)||p.warpRangeCycles<=0||!std::isfinite(p.effectiveLambdaMaterial)||p.effectiveLambdaMaterial<=0||!std::isfinite(p.effectiveLambdaCanvasPx)||p.effectiveLambdaCanvasPx<=0)return 400;
 // Check the actual pre-invert coordinate expressions against GLSL signed-int conversion domain.
 const auto intDomain=[](double x){const float f=static_cast<float>(x);return std::isfinite(f)&&double(f)>=-2147483648.0&&double(f)<2147483648.0;};
 for(size_t k=0;k<3;k++){
  if(!std::isfinite(static_cast<float>(p.bandMinYCanvasPx[k]))||!std::isfinite(static_cast<float>(p.bandMaxYCanvasPx[k])))return 400;
  for(double y:{p.canvasBox[1],p.canvasBox[1]+p.canvasBox[3]})if(!intDomain(y)||!intDomain(y-std::floor(p.bandMinYCanvasPx[k])))return 400;
  for(double x:{p.canvasBox[0],p.canvasBox[0]+p.canvasBox[2]})if(!intDomain(x-std::floor(p.grainOffsetCanvasPx[k]+.5)))return 400;
 }
 // GLSL integer texel coordinates must be representable; reject rather than wrap undefined casts.
 for(size_t c=0;c<128;c++)for(float x:{data[(128+c)*4+2],data[(128+c)*4+3],data[(256+c)*4]})if(std::abs(double(x)/p.warpRangeCycles*64)>=2147483648.0)return 400;
 return 0;
}

#ifndef P013_MATERIAL_PASS_CORE_ONLY
GLuint MaterialPass::Compile(GLenum type,const char* source){
 GLuint s=glCreateShader(type);glShaderSource(s,1,&source,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
 if(!ok){char log[2048]{};glGetShaderInfoLog(s,sizeof(log),nullptr,log);glDeleteShader(s);return 0;}return s;
}
Status MaterialPass::Create(const ShaderSources& sources,uint64_t context){
 Reset(true);epoch=context;auto v=Compile(GL_VERTEX_SHADER,sources.vertex),f=Compile(GL_FRAGMENT_SHADER,sources.fragment);
 if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);errorStage="MATERIAL_COMPILE";Reset(true);return Status::SHADER_COMPILE_FAILED;}
 program_=glCreateProgram();glAttachShader(program_,v);glAttachShader(program_,f);glLinkProgram(program_);glDeleteShader(v);glDeleteShader(f);
 GLint linked=0;glGetProgramiv(program_,GL_LINK_STATUS,&linked);if(!linked){errorStage="MATERIAL_LINK";Reset(true);return Status::SHADER_COMPILE_FAILED;}
 const char* names[]={"u_tdata","u_grain","u_strata","u_resolution","u_box","u_canvasBox","u_capacity","u_visualInverted","u_lightEnabled","u_grainEnabled","u_strataEnabled","u_sparkEnabled","u_palette[0]","u_grainBase[0]","u_offsets","u_bandMin","u_bandMax","u_strengths","u_warpRange","u_lightDir","u_light","u_lightDepth","u_baseStrip","u_useBaseStrip","u_viewportOriginX"};
 for(size_t i=0;i<25;i++){uniforms_[i]=glGetUniformLocation(program_,names[i]);if(i<22&&uniforms_[i]<0){errorStage="MATERIAL_UNIFORM";Reset(true);return Status::SHADER_COMPILE_FAILED;}}
 glGenVertexArrays(1,&vao_);glGenTextures(1,&dataTexture_);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,dataTexture_);
 glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA32F,128,3);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 error=glGetError();if(error!=GL_NO_ERROR||!vao_||!dataTexture_){errorStage="MATERIAL_RESOURCE";Reset(true);return Status::GL_RESOURCE_FAILED;}
 textureAllocCount++;errorStage="NONE";return Status::OK;
}
Status MaterialPass::Draw(const MaterialFrame& m,const LightParams& l,const MaterialParams& p,const float* data,const std::array<uint32_t,2>& textures,uint32_t baseStrip,int viewportOriginX){
 if(!program_||!dataTexture_||!textures[0]||!textures[1]||epoch!=m.contextEpoch||
   (baseStrip&& (uniforms_[22]<0||uniforms_[23]<0||uniforms_[24]<0))){errorStage="MATERIAL_NOT_READY";return Status::GL_RESOURCE_FAILED;}
 double t=NowMs();const auto& f=m.frame;
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,dataTexture_);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
 glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,128,3,GL_RGBA,GL_FLOAT,data);error=glGetError();uploadMs=NowMs()-t;
 if(error!=GL_NO_ERROR){errorStage="MATERIAL_TDATA_UPLOAD";return Status::GL_RESOURCE_FAILED;}uploadCount++;
 glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D_ARRAY,textures[0]);glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D_ARRAY,textures[1]);
 glActiveTexture(GL_TEXTURE3);glBindTexture(GL_TEXTURE_2D_ARRAY,baseStrip);
 glViewport(0,0,GLsizei(f.surfaceWidthPx),GLsizei(f.surfaceHeightPx));glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);
 glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
 // T68 candidate sand layer: binary coverage over preserves previously drawn scene layers. Caller owns clears.
 glUseProgram(program_);glBindVertexArray(vao_);
 for(int i=0;i<3;i++)glUniform1i(uniforms_[i],i);
 glUniform2f(uniforms_[3],f.surfaceWidthPx,f.surfaceHeightPx);glUniform4f(uniforms_[4],f.boxXPx,f.boxYPx,f.boxWidthPx,f.boxHeightPx);
 glUniform4f(uniforms_[5],p.canvasBox[0],p.canvasBox[1],p.canvasBox[2],p.canvasBox[3]);glUniform1f(uniforms_[6],f.capacity);
 glUniform1i(uniforms_[7],f.visualInverted);glUniform1i(uniforms_[8],m.lightEnabled);glUniform1i(uniforms_[9],p.grainEnabled);glUniform1i(uniforms_[10],p.strataEnabled);glUniform1i(uniforms_[11],p.sparkEnabled);
 float palette[6],grain[9];for(int i=0;i<6;i++)palette[i]=p.paletteBaseSrgb[i];for(int i=0;i<9;i++)grain[i]=p.grainBaseSrgb[i];
 glUniform3fv(uniforms_[12],2,palette);glUniform3fv(uniforms_[13],3,grain);
 glUniform3f(uniforms_[14],p.grainOffsetCanvasPx[0],p.grainOffsetCanvasPx[1],p.grainOffsetCanvasPx[2]);
 glUniform3f(uniforms_[15],p.bandMinYCanvasPx[0],p.bandMinYCanvasPx[1],p.bandMinYCanvasPx[2]);glUniform3f(uniforms_[16],p.bandMaxYCanvasPx[0],p.bandMaxYCanvasPx[1],p.bandMaxYCanvasPx[2]);
 glUniform3f(uniforms_[17],p.aB,p.aC,p.aC3);glUniform1f(uniforms_[18],p.warpRangeCycles);
 glUniform3f(uniforms_[19],l.lightX,l.lightY,l.lightZ);glUniform4f(uniforms_[20],l.la,l.ld,l.lmin,l.lmax);glUniform2f(uniforms_[21],l.lref,l.ellLightMaterial);
 if(uniforms_[22]>=0)glUniform1i(uniforms_[22],3);
 if(uniforms_[23]>=0)glUniform1i(uniforms_[23],baseStrip!=0);
 if(uniforms_[24]>=0)glUniform1i(uniforms_[24],viewportOriginX);
 t=NowMs();glDrawArrays(GL_TRIANGLES,0,3);drawMs=NowMs()-t;error=glGetError();errorStage=error==GL_NO_ERROR?"NONE":"MATERIAL_DRAW";
 return error==GL_NO_ERROR?Status::OK:Status::GL_DRAW_FAILED;
}
void MaterialPass::Reset(bool current){if(current){if(program_)glDeleteProgram(program_);if(vao_)glDeleteVertexArrays(1,&vao_);if(dataTexture_)glDeleteTextures(1,&dataTexture_);}program_=vao_=dataTexture_=0;epoch=0;}
#endif
}
