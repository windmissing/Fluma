#pragma once
#include "../program_cache/program_factory.h"
#include "../uniform_locations.h"
#include <array>
#include <cstring>
namespace grs::special536 {
struct Source {const char* name;const char* text;const char* sha;const char* abi;};
inline constexpr std::array<uint8_t,29> kFamily{{0,1,2,2,2,4,3,3,3,3,4,4,5,5,5,4,3,3,4,6,0,6,0,6,1,7,7,7,0}};
// Fixed owner-thread storage. Wire uploadedBytes retains its original logical
// command/staging meaning; physical uniform submissions are counted separately.
class Programs {
 struct Value {std::array<char,64> name{};std::array<uint32_t,2> words{};uint32_t type=0;uint64_t version=0;};
 std::array<GLuint,8> ids_{};
 std::array<UniformLocations,8> locations_{};
 std::array<Value,64> values_{};
 std::array<std::array<uint64_t,64>,8> applied_{};
 uint32_t count_=0;uint64_t version_=0,physicalBytes_=0,switches_=0;
 bool valid_=true;
public:
 bool Live()const{return ids_[0]!=0;}
 GLuint Id(uint32_t family)const{return family<8?ids_[family]:0;}
 uint64_t PhysicalUniformBytes()const{return physicalBytes_;}
 uint64_t Switches()const{return switches_;}
 bool Create(const std::array<Source,8>& sources,const SubmitObserver& observer,GLenum& error){
  if(Live())return false;
  for(size_t i=0;i<8;++i){
   const auto& source=sources[i];
   if(!programcache::Build(2,source.text,source.sha,source.abi,ids_[i],error,observer))return false;
   auto at=programcache::pending.find(ids_[i]);if(at!=programcache::pending.end())at->second.role=source.name;
   if(!programcache::Reflect(locations_[i],ids_[i]))return false;
   if(locations_[i].Find("uPass")<0)return false;
   if(programcache::Prepared(ids_[i])!=GL_NO_ERROR)return false;
  }
  return true;
 }
 void ResetValues(){count_=0;version_=0;valid_=true;values_={};applied_={};}
 void Put(const char* name,uint32_t a,uint32_t b=0,uint32_t type=0){
  if(!valid_||std::strlen(name)>=64||version_==UINT64_MAX){valid_=false;return;}
  size_t slot=0;for(;slot<count_;++slot)if(std::strcmp(values_[slot].name.data(),name)==0)break;
  if(slot==count_){if(count_==values_.size()){valid_=false;return;}++count_;std::strcpy(values_[slot].name.data(),name);}
  auto& value=values_[slot];value.words={a,b};value.type=type;value.version=++version_;
 }
 bool Bind(uint32_t stage){
  if(!valid_||stage>=kFamily.size())return false;
  const uint32_t f=kFamily[stage];if(!ids_[f])return false;
  glUseProgram(ids_[f]);++switches_;
  for(size_t i=0;i<count_;++i){const auto& value=values_[i];if(applied_[f][i]==value.version)continue;
   const auto location=locations_[f].Find(value.name.data());
   if(location>=0){if(value.type==1){glUniform1i(location,GLint(value.words[0]));physicalBytes_+=4;}
    else if(value.type==2){glUniform2ui(location,value.words[0],value.words[1]);physicalBytes_+=8;}
    else{glUniform1ui(location,value.words[0]);physicalBytes_+=4;}}
   applied_[f][i]=value.version;
  }
  return true;
 }
 void Warmup(uint64_t begin,uint64_t end,GLenum error){for(auto id:ids_)programcache::Warmup(id,begin,end,error);}
 void Release(bool current){for(auto id:ids_)if(id){programcache::Forget(id);if(current)glDeleteProgram(id);}ids_={};locations_={};ResetValues();}
};
}
