#include "single_sand_pass.h"
#include "single_sand_shaders.h"
#include "material_uploader.h"
#include <algorithm>
#include <cmath>
#include <utility>
namespace p013 {
namespace {
GLuint Compile(GLenum type,const char* text){
 GLuint s=glCreateShader(type);glShaderSource(s,1,&text,nullptr);glCompileShader(s);
 GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(s);return 0;}return s;
}
void Params(){
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
}
void Unpack(int alignment){
 glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);
 glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
}
float SurfaceAt(const SingleSandPacket& p,double column){
 const float c=std::clamp(float(column),0.f,127.f);
 const int left=std::min(126,int(c));
 const float t=c-left;
 return p.profile[left]*(1-t)+p.profile[left+1]*t;
}
const char* BedSupport(const SingleSandPacket& p){
 if(*std::max_element(p.profile.begin(),p.profile.begin()+128)==0)return "EMPTY_BED";
 const auto& b=p.box;
 const auto top=std::max(0.,std::ceil(b[1]-.5)+.5);
 const auto bottom=std::min(p.height-.5,std::ceil(b[1]+b[3]-.5)-.5);
 if(top>bottom)return "SUBPIXEL_BED";
 const float y=float((p.inverted?top:bottom)-b[1]);
 const float q=p.inverted?float(b[3])-y:y;
 const float h=(1-q/float(b[3]))*float(p.capacity);
 // A piecewise-linear profile reaches its grid maximum at a pixel adjacent
 // to a knot or the clipped viewport edge. No framebuffer read is needed.
 const auto hasBed=[&](double x){
  if(x<0||x>=p.width||x<b[0]||x>=b[0]+b[2])return false;
  const float local=float(x-b[0]);
  const float mirrored=p.inverted?float(b[2])-local:local;
  return h>=0&&h<SurfaceAt(p,mirrored/float(b[2])*128);
 };
 if(hasBed(.5)||hasBed(p.width-.5))return "BED_PIXEL_SUPPORT";
 for(int c=0;c<=128;c++){
  const double knot=b[0]+(p.inverted?128-c:c)*b[2]/128;
  const double near=std::floor(knot-.5)+.5;
  if(hasBed(near)||hasBed(near+1))return "BED_PIXEL_SUPPORT";
 }
 return "SUBPIXEL_BED";
}
}
const char* SingleSandShaderHash(){return kSingleSandHash;}
const char* SingleSandPass::Stage(uint64_t generation,const std::string& digest,const uint8_t* bytes){
 if(!generation||generation<=lastGeneration||pendingConfig.digest!=digest)return "SINGLE_CONFIG_ASSET_MISMATCH";
 if(MaterialSha256(bytes,65536)!=pendingConfig.grainHash)return "SINGLE_ASSET_HASH";
 SingleSandAsset candidate;candidate.config=pendingConfig;candidate.generation=generation;
 std::copy_n(bytes,65536,candidate.bytes.begin());
 // Replacement GL names are disposed by the owner during Draw or Reset.
 candidate.texture=pending.texture;pending=std::move(candidate);lastGeneration=generation;
 return "OK";
}
void SingleSandPass::Reset(bool current){
 targets.Reset(current);
 if(current){
  glDeleteProgram(program);glDeleteVertexArrays(1,&vao);glDeleteTextures(1,&profile);
  glDeleteTextures(1,&active.texture);glDeleteTextures(1,&pending.texture);
  glDeleteBuffers(1,&xy);glDeleteBuffers(1,&meta);
 }
 program=vao=profile=xy=meta=active.texture=pending.texture=0;
 epoch=shownSurface=shownFrame=0;shownToken.clear();
}
bool SingleSandPass::Ensure(uint64_t context){
 if(epoch==context&&program&&profile)return true;
 Reset(epoch==context);epoch=context;
 GLuint v=Compile(GL_VERTEX_SHADER,kSingleSandVert),f=Compile(GL_FRAGMENT_SHADER,kSingleSandFrag);
 if(!v||!f){glDeleteShader(v);glDeleteShader(f);return false;}
 program=glCreateProgram();glAttachShader(program,v);glAttachShader(program,f);glLinkProgram(program);
 glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
 if(!ok){glDeleteProgram(program);program=0;return false;}
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);
 glGenBuffers(1,&xy);glBindBuffer(GL_ARRAY_BUFFER,xy);glBufferData(GL_ARRAY_BUFFER,4096,nullptr,GL_STREAM_DRAW);
 glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,8,nullptr);glVertexAttribDivisor(0,1);
 glGenBuffers(1,&meta);glBindBuffer(GL_ARRAY_BUFFER,meta);glBufferData(GL_ARRAY_BUFFER,4096,nullptr,GL_STREAM_DRAW);
 glEnableVertexAttribArray(1);glVertexAttribIPointer(1,1,GL_UNSIGNED_INT,8,nullptr);glVertexAttribDivisor(1,1);
 glGenTextures(1,&profile);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,profile);
 glTexStorage2D(GL_TEXTURE_2D,1,GL_R32F,128,2);Params();
 if(glGetError()!=GL_NO_ERROR){Reset(true);return false;}return true;
}
SingleSandReceipt SingleSandPass::Draw(EglContext& egl,const SingleSandPacket& p){
 SingleSandReceipt out;
 if(epoch==p.context&&shownSurface==p.surface&&p.frame<=shownFrame){
  out.reason="SINGLE_PUBLICATION_MISMATCH";return out;
 }
 auto* asset=pending.generation==p.generation?&pending:active.generation==p.generation?&active:nullptr;
 if(!asset){out.reason="SINGLE_ASSET_NOT_READY";return out;}
 if(asset->config.session!=p.session||asset->config.revision!=p.revision||
    asset->config.digest!=p.digest||asset->config.grainHash!=p.grainHash){out.reason="SINGLE_CONFIG_ASSET_MISMATCH";return out;}
 out.lease=asset==&pending?"PENDING":"ACTIVE";
 out.bedSupport=BedSupport(p);
 if(p.enabled){
  for(int i=0;i<p.count;i++){
   if(p.meta[i*2]!=256)continue;
   if(p.xy[i*2+1]<SurfaceAt(p,p.xy[i*2]))out.occludedCenters++;
  }
 }
 out.encoding=egl.attachmentEncoding;out.colorSpace=egl.actualColorSpace;
 out.writeControl=egl.writeControlSupported;out.writeEnabled=egl.srgbWriteEnabled;
 if(out.encoding!=GL_SRGB||!out.colorSpace||!out.writeEnabled){out.reason="SINGLE_TARGET_UNSUPPORTED";return out;}
 out.stage="SHADER";
 if(!Ensure(p.context)){out.rawGl=glGetError();return out;}
 out.stage="TARGET";
 if(targets.width!=p.width||targets.height!=p.height)shownToken.clear();
 if(targets.Ensure(p.width,p.height,p.context)!=Status::OK){out.reason="SINGLE_TARGET_UNSUPPORTED";return out;}
 out.targets=targets.Bytes();
 const bool canReplay=!shownToken.empty()&&shownSurface==p.surface&&epoch==p.context;
 auto loc=[&](const char* n){return glGetUniformLocation(program,n);};
 auto present=[&](int index){
  glBindFramebuffer(GL_FRAMEBUFFER,0);glDisable(GL_BLEND);glUniform1i(loc("layer"),4);
  glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,targets.textures[index]);glDrawArrays(GL_TRIANGLES,0,3);out.drawCalls++;
  return glGetError()==GL_NO_ERROR;
 };
 auto fail=[&](){
  out.rawEgl=egl.error;
  out.actual.clear();out.actualDigest.clear();out.actualGrain.clear();out.actualProfile.clear();out.actualPb.clear();
  if(canReplay&&present(targets.lastGood)&&egl.Swap(p.frame)==Status::OK){
   out.replayed=true;
   out.state="STALE";out.actual=shownToken;out.actualDigest=shownDigest;out.actualGrain=shownGrain;
   out.actualProfile=shownProfile;out.actualPb=shownPb;
  }
  return out;
 };
 // Allocate all identity strings before the swap/commit boundary.
 auto token=p.token,digest=p.digest,grainHash=p.grainHash,display=p.displayHash,pb=p.pbHash;
 out.actual=token;out.actualDigest=digest;out.actualGrain=grainHash;out.actualProfile=display;out.actualPb=pb;
 out.stage="R8_UPLOAD";
 if(!asset->texture||asset==&pending){
  if(asset->texture)glDeleteTextures(1,&asset->texture);
  glGenTextures(1,&asset->texture);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,asset->texture);
  glTexStorage2D(GL_TEXTURE_2D,1,GL_R8,256,256);Params();Unpack(1);
  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,256,256,GL_RED,GL_UNSIGNED_BYTE,asset->bytes.data());out.uploaded+=65536;
  if((out.rawGl=glGetError())!=GL_NO_ERROR){glDeleteTextures(1,&asset->texture);asset->texture=0;
   out.actual.clear();out.actualDigest.clear();out.actualGrain.clear();out.actualProfile.clear();out.actualPb.clear();return out;}
 }
 out.stage="PROFILE_UPLOAD";
 glBindFramebuffer(GL_FRAMEBUFFER,targets.fbos[targets.candidate]);glViewport(0,0,p.width,p.height);
 glDisable(GL_BLEND);glDisable(GL_DITHER);glDisable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);
 glDisable(GL_STENCIL_TEST);glDisable(GL_CULL_FACE);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_SAMPLE_COVERAGE);
 glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
 glGetIntegerv(GL_SAMPLES,&out.samples);GLint range[2];glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_HIGH_FLOAT,range,&out.highp);
 glUseProgram(program);glBindVertexArray(vao);Unpack(4);
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,profile);
 glTexSubImage2D(GL_TEXTURE_2D,0,0,0,128,2,GL_RED,GL_FLOAT,p.profile.data());
 glBindBuffer(GL_ARRAY_BUFFER,xy);glBufferSubData(GL_ARRAY_BUFFER,0,p.count*8,p.xy.data());
 glBindBuffer(GL_ARRAY_BUFFER,meta);glBufferSubData(GL_ARRAY_BUFFER,0,p.count*8,p.meta.data());
 out.uploaded+=1024+p.count*16;
 out.textureCount=3+bool(active.texture)+bool(pending.texture);
 const auto& c=asset->config;const auto& v=c.values;
 glUniform1i(loc("profile"),0);glUniform1i(loc("grain"),1);glUniform1i(loc("frameTexture"),2);
 glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,asset->texture);
 glUniform2f(loc("viewport"),p.width,p.height);glUniform4f(loc("box"),p.box[0],p.box[1],p.box[2],p.box[3]);
 glUniform1i(loc("inverted"),p.inverted);glUniform1f(loc("capacity"),p.capacity);
 glUniform1f(loc("radius"),v[6]*p.scale);glUniform1f(loc("scale"),p.scale);glUniform1f(loc("depth"),v[10]);
 glUniform1f(loc("grainGain"),v[8]);glUniform1f(loc("surfaceGain"),v[9]);glUniform1f(loc("coreGain"),v[14]);glUniform1f(loc("rimGain"),v[15]);
 glUniform3fv(loc("albedo"),1,c.albedo.data());glUniform3fv(loc("background"),1,c.background.data());glUniform3fv(loc("lightDir"),1,c.light.data());
 if((out.rawGl=glGetError())!=GL_NO_ERROR){out.actual.clear();return fail();}
 const char* stages[]={"BACKGROUND","BED","PARTICLES","FOREGROUND"};
 for(int layer=0;layer<4;layer++){
  out.stage=stages[layer];glUniform1i(loc("layer"),layer);
  if(layer==3)glEnable(GL_BLEND);
  if(layer==2){if(p.enabled){glDrawArraysInstanced(GL_TRIANGLES,0,6,p.count);out.drawCalls++;}}
  else{glDrawArrays(GL_TRIANGLES,0,3);out.drawCalls++;}
  if((out.rawGl=glGetError())!=GL_NO_ERROR){out.actual.clear();return fail();}
 }
 out.stage="PRESENT";
 if(!present(targets.candidate)){out.actual.clear();return fail();}
 out.stage="SWAP";
 if(egl.Swap(p.frame)!=Status::OK){out.actual.clear();return fail();}
 targets.Commit();shownSurface=p.surface;shownFrame=p.frame;
 shownToken.swap(token);shownDigest.swap(digest);shownGrain.swap(grainHash);shownProfile.swap(display);shownPb.swap(pb);
 if(asset==&pending){std::swap(active,pending);pending.generation=0;}
 out.swapped=true;out.lease="COMMITTED";out.state="CURRENT";out.reason="OK";out.stage="COMMIT";return out;
}
}
