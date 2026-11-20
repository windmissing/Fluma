#pragma once
#include <array>
#include <sstream>
#include "program_cache/sha256.h"
namespace t496shadow {
// CPU-observed arguments/returns only; no additional GL reads. Fixed tables.
struct Object {uint32_t name=0;uint64_t generation=0;uint64_t bytes=0;std::string source;std::array<uint32_t,8> attached{};};
inline std::array<std::pair<std::string,std::string>,6> roles{};
inline std::array<Object,128> shaders{},programs{};
inline std::array<Object,512> buffers{};
inline std::array<std::pair<std::string,std::string>,128> binaries{};
inline uint64_t generation=0;inline uint32_t current=0;inline thread_local uintptr_t context=0;
inline std::array<uint32_t,4> bindings{};
inline std::array<std::pair<uint32_t,uint32_t>,8> targets{};
struct Uniform {uint32_t program=0;int location=-1;std::string value;};
inline std::array<Uniform,256> uniforms{};inline bool complete=true;
template<size_t N>Object* find(std::array<Object,N>& table,uint32_t name,bool create=false){if(!name)return nullptr;for(auto& x:table)if(x.name==name)return &x;if(create)for(auto& x:table)if(!x.name){x={};x.name=name;x.generation=++generation;return &x;}complete=false;return nullptr;}
inline std::string hash(const char* data,size_t bytes){p011gpu::Sha256 s;s.update(data,bytes);auto a=s.finish();static const char* hex="0123456789abcdef";std::string o;for(auto b:a){o+=hex[b>>4];o+=hex[b&15];}return o;}
inline void source(uint32_t name,int count,const char*const* strings,const int* lengths){p011gpu::Sha256 h;for(int i=0;i<count;++i){if(!strings||!strings[i]){complete=false;return;}const size_t n=lengths&&lengths[i]>=0?size_t(lengths[i]):std::strlen(strings[i]);h.update(strings[i],n);}auto a=h.finish();static const char* hex="0123456789abcdef";std::string out;for(auto b:a){out+=hex[b>>4];out+=hex[b&15];}if(auto* x=find(shaders,name))x->source=out;}
inline void bind(uint32_t target,uint32_t name){for(auto& x:targets)if(!x.first||x.first==target){x={target,name};return;}complete=false;}
inline void allocate(uint32_t target,uint64_t bytes){for(auto& x:targets)if(x.first==target){if(auto* b=find(buffers,x.second))b->bytes=bytes;return;}complete=false;}
inline void uniform(int location,const std::string& value){if(location<0)return;for(auto& u:uniforms)if(u.location<0||(u.program==current&&u.location==location)){u={current,location,value};return;}complete=false;}
inline std::string describe(){std::ostringstream o;o<<"observedOnly=1 complete="<<complete<<" context="<<context<<" program="<<current;auto* p=find(programs,current);if(p){o<<" generation="<<p->generation;}o<<" programSources="<<(p?p->source:"UNKNOWN");bool role=false;if(p)for(auto& r:roles)if(!r.first.empty()&&p->source==r.first){o<<" role="<<r.second;role=true;}if(!role)o<<" role=UNKNOWN";for(size_t i=0;i<4;++i){auto* b=find(buffers,bindings[i]);o<<" bind"<<i<<"="<<bindings[i]<<":"<<(b?b->generation:0)<<":"<<(b?b->bytes:0);}for(auto& u:uniforms)if(u.program==current&&u.location>=0)o<<" u"<<u.location<<"="<<u.value;return o.str();}
}
