#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

namespace t496map {
// Host wrapper observations. Zero generations identify unknown provenance.
struct Binding {uint32_t target=0,name=0;uint64_t generation=0,life=0;};
struct IndexedBinding {Binding object;uint32_t index=0;int64_t offset=0,size=0;};
struct Context {
 uintptr_t display=0,handle=0;
 uint64_t generation=0,group=0;
 std::thread::id owner;
 std::array<Binding,16> bindings{};
 std::array<IndexedBinding,64> indexed{};
};
struct Buffer {
 uint64_t group=0,life=0,storage=0,size=0,map=0,mapContext=0,mapBinding=0;
 uint32_t name=0;
};
struct Identity {
 uint64_t context=0,group=0,life=0,storage=0,binding=0,size=0,token=0;
 uint32_t name=0,target=0;
 bool known() const {return context&&group&&life&&storage&&binding;}
};
inline std::mutex lock;
inline std::array<Context,32> contexts{};
inline std::array<Buffer,1024> buffers{};
inline uint64_t next=0;
inline thread_local uint64_t current=0;
inline Context* findContext(uintptr_t display,uintptr_t handle) {
 for(auto& c:contexts)if(c.generation&&c.display==display&&c.handle==handle)return &c;
 return nullptr;
}
inline Context* active() {
 if(current)for(auto& c:contexts)
  if(c.generation==current&&c.owner==std::this_thread::get_id())return &c;
 return nullptr;
}
inline void created(uintptr_t display,uintptr_t handle,uintptr_t share) {
 if(!handle)return;
 std::lock_guard<std::mutex> guard(lock);
 const auto* parent=share?findContext(display,share):nullptr;
 const auto generation=++next;
 const auto group=share?(parent?parent->group:0):generation;
 for(auto& c:contexts)if(!c.generation){c={display,handle,generation,group,{},{},{}};return;}
}
inline void madeCurrent(uintptr_t display,uintptr_t handle) {
 std::lock_guard<std::mutex> guard(lock);
 if(auto* previous=active())previous->owner={};
 auto* context=findContext(display,handle);
 if(context)context->owner=std::this_thread::get_id();
 current=context?context->generation:0;
}
inline void retired(uintptr_t display,uintptr_t handle,bool all=false) {
 std::lock_guard<std::mutex> guard(lock);
 for(auto& c:contexts)if(c.display==display&&(all||c.handle==handle))c={};
}
inline Buffer* findBuffer(uint64_t group,uint32_t name) {
 if(group&&name)for(auto& b:buffers)if(b.group==group&&b.name==name)return &b;
 return nullptr;
}
inline void generated(uint32_t name) {
 std::lock_guard<std::mutex> guard(lock);
 auto* c=active();
 if(!c||!c->group||!name||findBuffer(c->group,name))return;
 for(auto& b:buffers)if(!b.name){b={c->group,++next,0,0,0,0,0,name};return;}
}
inline void deleted(uint32_t name) {
 std::lock_guard<std::mutex> guard(lock);
 auto* c=active();
 if(!c)return;
 if(auto* b=findBuffer(c->group,name))*b={};
 for(auto& alias:c->bindings)if(alias.name==name)alias={};
 for(auto& alias:c->indexed)if(alias.object.name==name)alias={};
}
inline void bound(uint32_t target,uint32_t name,const uint32_t* index=nullptr,
                  int64_t offset=0,int64_t size=0) {
 std::lock_guard<std::mutex> guard(lock);
 auto* c=active();
 if(!c)return;
 const auto* object=findBuffer(c->group,name);
 const Binding binding{target,name,++next,object?object->life:0};
 for(auto& b:c->bindings)if(!b.target||b.target==target){b=binding;break;}
 if(index)for(auto& b:c->indexed)if(!b.object.target||(b.object.target==target&&b.index==*index)){
  b={binding,*index,offset,size};
  return;
 }
}
inline Identity observeLocked(uint32_t target) {
 Identity id{};
 id.target=target;
 if(auto* c=active()){
  id.context=c->generation;
  id.group=c->group;
  for(const auto& b:c->bindings)if(b.target==target){
   id.binding=b.generation;
   id.name=b.name;
   if(auto* object=findBuffer(c->group,b.name);object&&object->life==b.life){
    id.life=object->life;id.storage=object->storage;id.size=object->size;id.token=object->map;
   }
   break;
  }
 }
 return id;
}
inline Identity observe(uint32_t target) {
 std::lock_guard<std::mutex> guard(lock);
 return observeLocked(target);
}
inline void allocated(uint32_t target,int64_t size) {
 std::lock_guard<std::mutex> guard(lock);
 const auto id=observeLocked(target);
 if(auto* b=findBuffer(id.group,id.name);b&&b->life==id.life){
  b->storage=size>=0?++next:0;b->size=size>=0?uint64_t(size):0;b->map=0;
 }
}
inline bool same(const Identity& a,const Identity& b) {
 return a.known()&&b.known()&&a.context==b.context&&a.target==b.target&&
        a.group==b.group&&a.life==b.life&&a.storage==b.storage&&a.binding==b.binding;
}
inline uint64_t mapped(const Identity& captured,int64_t offset,int64_t length) {
 std::lock_guard<std::mutex> guard(lock);
 const auto actual=observeLocked(captured.target);
 if(!same(captured,actual)||offset<0||length<=0||uint64_t(offset)>captured.size||
    uint64_t(length)>captured.size-uint64_t(offset)||actual.token)return 0;
 auto* b=findBuffer(actual.group,actual.name);
 b->mapContext=actual.context;b->mapBinding=actual.binding;
 return b->map=++next;
}
inline bool unmapped(const Identity& captured) {
 std::lock_guard<std::mutex> guard(lock);
 const auto actual=observeLocked(captured.target);
 if(!same(captured,actual)||!captured.token||captured.token!=actual.token)return false;
 auto* b=findBuffer(actual.group,actual.name);
 const bool matched=b->mapContext==actual.context&&b->mapBinding==actual.binding;
 b->map=0;
 return matched;
}
inline std::string describe(const Identity& i) {
 return " contextGeneration="+std::to_string(i.context)+" shareGroup="+std::to_string(i.group)+
        " name="+std::to_string(i.name)+" life="+std::to_string(i.life)+
        " storage="+std::to_string(i.storage)+" binding="+std::to_string(i.binding)+
        " storageBytes="+std::to_string(i.size)+" mapToken="+std::to_string(i.token)+
        " identity="+(i.known()?"WRAPPER_OBSERVED":"UNKNOWN");
}
}
