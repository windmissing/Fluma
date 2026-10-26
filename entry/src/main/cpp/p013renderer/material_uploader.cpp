#include "material_uploader.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <new>
#include <cmath>
#include <limits>
namespace p013 {
namespace {
uint32_t R(uint32_t x,int n){return (x>>n)|(x<<(32-n));}
const uint32_t K[64]={0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
}
std::string MaterialSha256(const uint8_t* bytes,size_t n){
 uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
 const size_t blocks=(n+9+63)/64; const uint64_t bits=static_cast<uint64_t>(n)*8;
 for(size_t block=0;block<blocks;block++){
  uint8_t data[64]{};for(size_t j=0;j<64;j++){size_t i=block*64+j;if(i<n)data[j]=bytes[i];else if(i==n)data[j]=128;else if(i>=blocks*64-8)data[j]=static_cast<uint8_t>(bits>>(8*(blocks*64-1-i)));}
  uint32_t w[64]{};for(int j=0;j<16;j++)w[j]=(uint32_t(data[4*j])<<24)|(uint32_t(data[4*j+1])<<16)|(uint32_t(data[4*j+2])<<8)|data[4*j+3];
  for(int j=16;j<64;j++){uint32_t a=w[j-15],b=w[j-2];w[j]=w[j-16]+(R(a,7)^R(a,18)^(a>>3))+w[j-7]+(R(b,17)^R(b,19)^(b>>10));}
  uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],z=h[7];
  for(int j=0;j<64;j++){uint32_t t=z+(R(e,6)^R(e,11)^R(e,25))+((e&f)^(~e&g))+K[j]+w[j];uint32_t u=(R(a,2)^R(a,13)^R(a,22))+((a&b)^(a&c)^(b&c));z=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+u;}
  uint32_t v[8]={a,b,c,d,e,f,g,z};for(int j=0;j<8;j++)h[j]+=v[j];
 }
 char out[65]{};for(int j=0;j<8;j++)std::snprintf(out+j*8,9,"%08x",h[j]);return out;
}
namespace {
bool Digest(const std::string& s){return s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
std::string Fingerprint(const MaterialDesc& d){
 std::string bytes;
 auto u=[&](uint64_t v){for(int i=0;i<8;i++)bytes.push_back(char(v>>(8*i)));};
 auto text=[&](const std::string& t){u(t.size());bytes+=t;};
 auto number=[&](double v){uint64_t bits=0;std::memcpy(&bits,&v,8);u(bits);};
 text("P1_ALBEDO_DELTA_V1|E2_GAUSS_COL6_R18_P1_V2|LIN_SRGB_REINHARD_V1|P1_TEXEL_CANVAS_PX_V1|CONTINUOUS_COLUMN_WARP_V1|2|1|256|256|64|256|3|SRGB8_STRAIGHT_OPAQUE");
 text(d.bundleId);text(d.grainHash);text(d.strataHash);text(d.metadata);text(d.paletteHash);text(d.generatorVersion);
 u(d.revision);for(auto v:d.grainBase)number(v);number(d.lambdaMaterial);number(d.lambdaCanvas);number(d.warpRange);number(d.seed);number(d.geometryEpoch);
 return MaterialSha256(reinterpret_cast<const uint8_t*>(bytes.data()),bytes.size());
}
}
MaterialConfigReceipt MaterialUploader::BindConfiguration(uint64_t session,uint64_t revision,const std::string& digest){
 MaterialConfigReceipt r;r.sessionId=boundSession_;r.bindingId=bindingId_;r.revision=boundRevision_;r.digest=boundDigest_;
 if(!session||session>9007199254740991ULL||revision>9007199254740991ULL||!Digest(digest)){r.code=400;r.errorStage="configBinding";return r;}
 if(bindingId_&&session!=boundSession_){r.code=400;r.errorStage="sessionId";return r;}
 if(bindingId_&&revision==boundRevision_){if(digest!=boundDigest_){r.code=402;r.errorStage="appearanceConfigRevision";}return r;}
 if(bindingId_==9007199254740991ULL){r.code=400;r.errorStage="bindingId";return r;}
 boundSession_=session;boundRevision_=revision;boundDigest_=digest;++bindingId_;
 r.sessionId=session;r.bindingId=bindingId_;r.revision=revision;r.digest=digest;return r;
}
MaterialReceipt MaterialUploader::CheckConfiguration(const MaterialDesc& d)const{
 auto r=Receipt(nullptr,bindingId_?402:401,bindingId_?"appearanceConfigRevision":"configBinding");
 r.assetGeneration=d.generation;r.requestedRevision=d.revision;r.appearanceConfigRevision=d.revision;r.state="FAILED";
 if(bindingId_&&d.revision==boundRevision_){r.code=0;r.errorStage="NONE";}return r;
}
MaterialReceipt MaterialUploader::InvalidBudget()const{return Receipt(pending_?pending_.get():active_.get(),400,"byteBudget");}
void MaterialUploader::ClearSession(){Drop(active_);Drop(pending_);boundSession_=bindingId_=boundRevision_=highWater_=pumps_=discarded_=0;boundDigest_.clear();}
void MaterialUploader::Drop(std::unique_ptr<Generation>& g,bool current){if(g){gpu_.Delete(g->textures,current);g.reset();}}
MaterialReceipt MaterialUploader::Receipt(const Generation* g,int code,const char* stage,size_t bytes)const{
 MaterialReceipt r;r.boundRevision=boundRevision_;r.bindingId=bindingId_;r.configurationDigest=boundDigest_;r.requestedRevision=g?g->desc.revision:0;r.code=code;r.errorStage=stage;r.uploadedBytes=bytes;r.pumpCount=pumps_;r.discardedGeneration=discarded_;
 if(g){r.assetGeneration=g->desc.generation;r.appearanceConfigRevision=g->desc.revision;r.contextEpoch=g->desc.contextEpoch;r.state=g->state;r.receivedBytes=g->grain.size()+g->strata.size();r.pendingBytes=kMaterialBytes-g->uploaded;r.frameToken=g->token;}
 r.activeGeneration=active_?active_->desc.generation:0;r.pendingGeneration=pending_?pending_->desc.generation:0;
 for(auto* x:{active_.get(),pending_.get()})if(x){r.cpuBytes+=x->grain.size()+x->strata.size();for(auto t:x->textures)if(t)r.textureLiveCount++;}
 return r;
}
MaterialReceipt MaterialUploader::Stage(const MaterialDesc& d,const uint8_t* grain,size_t gn,const uint8_t* strata,size_t sn){
 auto failed=[&](const char* stage){auto r=Receipt(nullptr,400,stage);r.state="FAILED";r.assetGeneration=d.generation;r.requestedRevision=d.revision;return r;};
 if(d.bundleId.empty()||!d.generation||d.metadata.empty())return failed("metadata");
 auto configuration=CheckConfiguration(d);if(configuration.code)return configuration;
 if(!grain||!strata||gn!=kMaterialGrainBytes||sn!=kMaterialStrataBytes)return failed("byteLength");
 if(MaterialSha256(grain,gn)!=d.grainHash||MaterialSha256(strata,sn)!=d.strataHash)return failed("hash");
 for(size_t i=3;i<gn;i+=4)if(grain[i]!=255)return failed("alpha");
 for(size_t i=3;i<sn;i+=4)if(strata[i]!=255)return failed("alpha");
 const auto fingerprint=Fingerprint(d);
 for(auto* resident:{active_.get(),pending_.get()})if(resident&&resident->desc.generation==d.generation){
  if(resident->fingerprint!=fingerprint||resident->bindingId!=bindingId_||resident->desc.surfaceEpoch!=d.surfaceEpoch||resident->desc.contextEpoch!=d.contextEpoch)return failed("identity");
  return Receipt(resident);
 }
 if(d.generation<=highWater_)return failed("identity");
 // Validate and copy before replacing pending; a rejected stage cannot alter either generation.
 std::unique_ptr<Generation> next;
 try{next=std::make_unique<Generation>();next->desc=d;next->fingerprint=fingerprint;next->bindingId=bindingId_;next->bindingDigest=boundDigest_;next->grain.assign(grain,grain+gn);next->strata.assign(strata,strata+sn);}catch(const std::bad_alloc&){return failed("allocation");}
 if(pending_){discarded_=pending_->desc.generation;Drop(pending_);}pending_=std::move(next);highWater_=d.generation;return Receipt(pending_.get());
}
MaterialReceipt MaterialUploader::Pump(double requestedBudget,uint64_t surface,uint64_t context){
 if(!std::isfinite(requestedBudget)||requestedBudget<1||requestedBudget>65536||std::floor(requestedBudget)!=requestedBudget)return InvalidBudget();
 const size_t budget=static_cast<size_t>(requestedBudget);
 if(!pending_)return Receipt(nullptr,401,"ASSET_NOT_READY");
 auto& p=*pending_;
 if(p.desc.surfaceEpoch!=surface||p.desc.contextEpoch!=context){discarded_=p.desc.generation;Drop(pending_,p.desc.contextEpoch==context);auto r=Receipt(nullptr,400,"epoch");r.assetGeneration=discarded_;r.state="DISCARDED";return r;}
 if(p.state=="READY")return Receipt(&p);
 if(p.state=="FAILED")return Receipt(&p,301,"upload_failed");
 pumps_++;
 if(!p.textures[0]&&!gpu_.Create(p.textures)){gpu_.Delete(p.textures,true);p.state="FAILED";return Receipt(&p,301,"texture_allocate");}
 size_t used=0;
 while(p.uploaded<kMaterialBytes){bool grain=p.uploaded<kMaterialGrainBytes;size_t stride=grain?1024:256;
  size_t offset=grain?p.uploaded:p.uploaded-kMaterialGrainBytes;size_t row=(offset/stride)%256,layer=offset/(stride*256);
  size_t rows=std::min<size_t>(256-row,(budget-used)/stride);if(!rows)break;
  const auto& bytes=grain?p.grain:p.strata;
  if(!gpu_.Upload(p.textures[grain?0:1],grain?256:64,static_cast<int>(layer),static_cast<int>(row),static_cast<int>(rows),bytes.data()+offset)){
   gpu_.Delete(p.textures,true);p.state="FAILED";return Receipt(&p,301,"glTexSubImage3D",used);}
  size_t n=rows*stride;p.uploaded+=n;used+=n;
 }
 p.state=p.uploaded==kMaterialBytes?"READY":"UPLOADING";return Receipt(&p,0,"NONE",used);
}
bool MaterialUploader::TargetMatches(uint64_t surface,uint64_t context)const{return pending_&&pending_->desc.surfaceEpoch==surface&&pending_->desc.contextEpoch==context;}
bool MaterialUploader::CanActivate(uint64_t generation,uint64_t epoch)const{return pending_&&pending_->state=="READY"&&pending_->desc.generation==generation&&pending_->desc.contextEpoch==epoch&&pending_->bindingId==bindingId_&&pending_->desc.revision==boundRevision_;}
MaterialReceipt MaterialUploader::Activate(uint64_t generation,uint64_t epoch,const std::string& token){
 if(pending_&&pending_->state=="READY"&&pending_->desc.generation==generation&&pending_->desc.contextEpoch==epoch&&pending_->bindingId!=bindingId_)return Receipt(pending_.get(),402,"appearanceConfigRevision");
 if(token.empty()||!CanActivate(generation,epoch))return Receipt(pending_.get(),401,"ASSET_NOT_READY");
 // NAPI calls are synchronous, owner/mutex serialized; all preceding draws have completed unbind.
 Drop(active_);active_=std::move(pending_);active_->state="ACTIVE";active_->token=token;return Receipt(active_.get());
}
MaterialReceipt MaterialUploader::Discard(uint64_t generation){if(pending_&&pending_->desc.generation==generation){discarded_=generation;Drop(pending_);auto r=Receipt(nullptr);r.assetGeneration=generation;r.state="DISCARDED";return r;}return Receipt(nullptr,401,"ASSET_NOT_READY");}
void MaterialUploader::Reset(bool current){if(active_)gpu_.Delete(active_->textures,current);if(pending_)gpu_.Delete(pending_->textures,current);if(pending_){discarded_=pending_->desc.generation;pending_.reset();}}
MaterialReceipt MaterialUploader::Restore(uint64_t surface,uint64_t context){
 if(!active_)return Receipt(nullptr,401,"ASSET_NOT_READY");
 std::unique_ptr<Generation> next;
 try{next=std::make_unique<Generation>(*active_);}catch(const std::bad_alloc&){return Receipt(active_.get(),400,"allocation");}
 next->desc.surfaceEpoch=surface;next->desc.contextEpoch=context;next->textures={};next->uploaded=0;next->state="CPU_STAGED";
 Drop(pending_);pending_=std::move(next);return Receipt(pending_.get());
}
MaterialReceipt MaterialUploader::Inspect()const{return Receipt(pending_?pending_.get():active_.get());}
const MaterialDesc* MaterialUploader::PendingDesc()const{return pending_?&pending_->desc:nullptr;}
const MaterialDesc* MaterialUploader::ActiveDesc()const{return active_?&active_->desc:nullptr;}
const std::array<uint32_t,2>* MaterialUploader::BindActive(uint64_t epoch)const{return active_&&active_->desc.contextEpoch==epoch&&active_->textures[0]&&active_->textures[1]?&active_->textures:nullptr;}
}

#ifndef P013_MATERIAL_CORE_ONLY
#include <GLES3/gl3.h>
namespace p013 {
namespace {
void MaterialPixelStore(){
 glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glPixelStorei(GL_UNPACK_ALIGNMENT,4);
 glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
 glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_IMAGE_HEIGHT,0);glPixelStorei(GL_UNPACK_SKIP_IMAGES,0);
}
}
bool MaterialGlGpu::Create(std::array<uint32_t,2>& textures){
 MaterialPixelStore();glGenTextures(2,textures.data());
 for(int i=0;i<2;i++){
  if(!textures[i])return false;
  glActiveTexture(GL_TEXTURE1+i);glBindTexture(GL_TEXTURE_2D_ARRAY,textures[i]);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY,1,GL_RGBA8,i==0?256:64,256,3);
  if(glGetError()!=GL_NO_ERROR)return false;
 }
 return true;
}
bool MaterialGlGpu::Upload(uint32_t texture,int width,int layer,int row,int rows,const uint8_t* bytes){
 MaterialPixelStore();glActiveTexture(width==256?GL_TEXTURE1:GL_TEXTURE2);
 glBindTexture(GL_TEXTURE_2D_ARRAY,texture);
 glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,row,layer,width,rows,1,GL_RGBA,GL_UNSIGNED_BYTE,bytes);
 return glGetError()==GL_NO_ERROR;
}
void MaterialGlGpu::Delete(std::array<uint32_t,2>& textures,bool current){
 if(current)glDeleteTextures(2,textures.data());textures={};
}
}
#endif
