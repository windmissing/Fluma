#include "base_strip_uploader.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <limits>
#ifndef P013_MATERIAL_CORE_ONLY
#include <GLES3/gl3.h>
#endif
namespace p013 {
#ifndef P013_MATERIAL_CORE_ONLY
bool BaseStripGlGpu::Create(uint32_t& out,int width){
 GLint maxWidth=0,maxLayers=0;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxWidth);glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS,&maxLayers);
 if(width<1||width>maxWidth||maxLayers<3)return false;
 glGenTextures(1,&out);if(!out)return false;glBindTexture(GL_TEXTURE_2D_ARRAY,out);
 glTexStorage3D(GL_TEXTURE_2D_ARRAY,1,GL_RGBA8UI,width,1,3);
 glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
 glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 if(glGetError()!=GL_NO_ERROR){Delete(out,true);return false;}return true;
}
bool BaseStripGlGpu::Upload(uint32_t texture,int layer,int offset,int pixels,const uint8_t* bytes){
 if(!texture||layer<0||layer>2||offset<0||pixels<1)return false;
 glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glPixelStorei(GL_UNPACK_ALIGNMENT,4);
 glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
 glBindTexture(GL_TEXTURE_2D_ARRAY,texture);
 glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,offset,0,layer,pixels,1,1,GL_RGBA_INTEGER,GL_UNSIGNED_BYTE,bytes);
 return glGetError()==GL_NO_ERROR;
}
void BaseStripGlGpu::Delete(uint32_t& texture,bool current){if(texture&&current)glDeleteTextures(1,&texture);texture=0;}
#else
bool BaseStripGlGpu::Create(uint32_t&,int){return false;}
bool BaseStripGlGpu::Upload(uint32_t,int,int,int,const uint8_t*){return false;}
void BaseStripGlGpu::Delete(uint32_t& texture,bool){texture=0;}
#endif
BaseStripReceipt BaseStripUploader::Result(int code,const char* stage)const{
 BaseStripReceipt r;r.code=code;r.stage=stage;r.activeSerial=active_.valid&&active_.ready?active_.desc.serial:0;
 r.pendingSerial=pending_.valid?pending_.desc.serial:0;r.discardedSerial=discarded_;
 r.restoreCount=restoreCount_;r.state=pending_.valid?(pending_.ready?"READY":pending_.uploaded?"UPLOADING":"CPU_STAGED"):
  active_.valid&&active_.ready?"ACTIVE":"DISCARDED";
 r.uploadedBytes=pending_.uploaded;r.pendingBytes=pending_.valid?pending_.bytes.size()-pending_.uploaded:0;
 r.cpuBytes=active_.bytes.size()+pending_.bytes.size();r.gpuBytes=size_t(textureWidth_[0]+textureWidth_[1])*12;
 r.textureLiveCount=size_t(bool(textures_[0]))+size_t(bool(textures_[1]));return r;
}
BaseStripReceipt BaseStripUploader::Inspect()const{return Result();}
BaseStripReceipt BaseStripUploader::Stage(const BaseStripDesc& d,const uint8_t* bytes,size_t length){
 if(held_||!d.session||!d.serial||!d.stateEpoch||!d.commit||!d.bindingId||!d.geometry||
 !d.context||!d.surface||d.width<1||d.height<1||d.width>INT32_MAX/12||
 !std::isfinite(d.boxX)||!std::isfinite(d.boxWidth)||d.boxWidth<=0||
 d.token.empty()||d.run.empty()||d.digest.size()!=64||d.profileHash.size()!=64||d.paletteHash.size()!=64||
 d.payloadHash.size()!=64||!bytes||length!=size_t(d.width)*12||
 d.representation!="P013_NATIVE_X_F64_BASEBYTE_V2"||d.evaluator!="P1_FROZEN_F64_EVAL_V1"||
 d.paletteRevision!="P1_ENCODED_PALETTE_F64_V1")return Result(400,"BASE_STRIP_DESCRIPTOR");
 if(MaterialSha256(bytes,length)!=d.payloadHash)return Result(400,"BASE_STRIP_HASH");
 for(size_t i=0;i<length;i+=4)if(bytes[i]>248||bytes[i+1]>248||bytes[i+2]>248||
    bytes[i+3]>1||(bytes[i+3]==0&&(bytes[i]||bytes[i+1]||bytes[i+2])))
    return Result(400,"BASE_STRIP_DOMAIN");
 for(int j=0;j<d.width;j++){
  const double x=double(d.originX)+double(j)+.5-d.boxX;
  const uint8_t valid=x>=0&&x<d.boxWidth?1:0;
  for(int k=0;k<3;k++)if(bytes[(size_t(k)*size_t(d.width)+size_t(j))*4+3]!=valid)
   return Result(400,"BASE_STRIP_COVERAGE");
 }
 if(active_.valid&&d.serial<=active_.desc.serial)return Result(402,"BASE_STRIP_SERIAL");
 if(pending_.valid&&d.serial<=pending_.desc.serial)return Result(402,"BASE_STRIP_SERIAL");
 Slot next;next.valid=true;next.ready=false;next.desc=d;next.bytes.assign(bytes,bytes+length);
 if(pending_.valid)discarded_=pending_.desc.serial;
 // Stage may run without an EGL context. Resize invalidation and GL deletion
 // happen in Pump, under the render owner's current context.
 if(active_.valid&&active_.desc.width!=d.width){active_=Slot{};activeSlot_=-1;}
 pending_=std::move(next);
 pendingSlot_=activeSlot_==0?1:0;return Result();
}
BaseStripReceipt BaseStripUploader::Pump(double budget,uint64_t surface,uint64_t context){
 if(!std::isfinite(budget)||budget<1||budget>65536||std::floor(budget)!=budget)return Result(400,"BASE_STRIP_BUDGET");
 if(!pending_.valid)return Result(401,"BASE_STRIP_NOT_STAGED");
 if(pending_.desc.surface!=surface||pending_.desc.context!=context)return Result(402,"BASE_STRIP_EPOCH");
 if(context_!=context){const bool restore=context_!=0;Reset(false);context_=context;if(restore)restoreCount_++;}
 const size_t take=std::min<size_t>(size_t(budget)/4*4,pending_.bytes.size()-pending_.uploaded);
 if(take==0)return Result();
 for(int i=0;i<2;i++)if(textures_[i]&&textureWidth_[i]!=pending_.desc.width){
  gpu_.Delete(textures_[i],true);textureWidth_[i]=0;
 }
 if(textures_[pendingSlot_]&&textureWidth_[pendingSlot_]!=pending_.desc.width){gpu_.Delete(textures_[pendingSlot_],true);textureWidth_[pendingSlot_]=0;}
 if(!textures_[pendingSlot_]){if(!gpu_.Create(textures_[pendingSlot_],pending_.desc.width))return Result(403,"BASE_STRIP_TEXTURE");
   textureWidth_[pendingSlot_]=pending_.desc.width;}
 size_t done=0;
 while(done<take){const size_t offset=pending_.uploaded+done;
  const int layer=int(offset/(size_t(pending_.desc.width)*4));
  const int x=int((offset/4)%size_t(pending_.desc.width));
  const int pixels=int(std::min<size_t>((take-done)/4,size_t(pending_.desc.width)-size_t(x)));
  if(!gpu_.Upload(textures_[pendingSlot_],layer,x,pixels,pending_.bytes.data()+offset))return Result(403,"BASE_STRIP_UPLOAD");
  done+=size_t(pixels)*4;
 }
 pending_.uploaded+=done;pending_.ready=pending_.uploaded==pending_.bytes.size();return Result();
}
bool BaseStripUploader::Acquire(const BaseStripDesc& d,BaseStripLease& lease){
 if(held_||!pending_.valid||!pending_.ready||!textures_[pendingSlot_])return false;
 const auto& p=pending_.desc;
 if(p.session!=d.session||p.serial!=d.serial||p.token!=d.token||p.run!=d.run||p.stateEpoch!=d.stateEpoch||
 p.commit!=d.commit||p.bindingId!=d.bindingId||p.digest!=d.digest||p.geometry!=d.geometry||
 p.context!=d.context||p.surface!=d.surface||p.width!=d.width||p.height!=d.height||
 p.originX!=d.originX||p.originY!=d.originY||std::memcmp(&p.boxX,&d.boxX,sizeof(double))!=0||
 std::memcmp(&p.boxWidth,&d.boxWidth,sizeof(double))!=0||
 p.mirror!=d.mirror||p.spark!=d.spark||p.profileHash!=d.profileHash||
 p.paletteHash!=d.paletteHash||p.payloadHash!=d.payloadHash||
 p.representation!=d.representation||p.evaluator!=d.evaluator||p.paletteRevision!=d.paletteRevision)return false;
 held_=true;lease.desc=&pending_.desc;lease.texture=textures_[pendingSlot_];lease.pending=true;return true;
}
void BaseStripUploader::CommitAfterSwap(BaseStripLease& lease)noexcept{
 if(!held_||!lease.pending||lease.desc!=&pending_.desc)return;
 active_=std::move(pending_);pending_=Slot{};activeSlot_=pendingSlot_;pendingSlot_=1-activeSlot_;
 held_=false;lease=BaseStripLease{};
}
void BaseStripUploader::Abort(BaseStripLease& lease)noexcept{held_=false;lease=BaseStripLease{};}
void BaseStripUploader::Reset(bool current){gpu_.Delete(textures_[0],current);gpu_.Delete(textures_[1],current);
 textureWidth_[0]=textureWidth_[1]=0;context_=0;activeSlot_=-1;pendingSlot_=0;held_=false;active_.ready=false;pending_.ready=false;pending_.uploaded=0;}
void BaseStripUploader::ClearSession(bool current){Reset(current);active_=Slot{};pending_=Slot{};discarded_=0;restoreCount_=0;}
}
