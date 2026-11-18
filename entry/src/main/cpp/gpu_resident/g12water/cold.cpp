// T576 (problem T567): G12 cold implementation.
#include "cold.h"
#include <cstring>
namespace grs::g12water {
namespace {
struct PageJson {
  char data[Cold::kPageUtf8Bytes];size_t length = 0;bool overflow = false;
  void Raw(const char* text,size_t bytes){if(length+bytes>sizeof(data)-1){overflow=true;return;}std::memcpy(data+length,text,bytes);length+=bytes;}
  void Number(uint32_t v){char s[16];const auto n=std::snprintf(s,sizeof(s),"%u",v);Raw(s,size_t(n));}
};
}
std::string Cold::Begin(uint64_t token,uint64_t now,uint32_t mode,const Transport& transport){
 const auto* profile=transport.SelectedProfile();
 if(!profile||!token||mode>2)return "{\"result\":\"INVALID\"}";
 for(auto& slot:slots_){
  if(slot.live&&slot.token==token&&now-slot.createdNs<kTtlNs)return "{\"result\":\"STALE_EXPORT\"}";
  if(slot.live&&now-slot.createdNs>=kTtlNs)slot.live=false;
 }
 bool free=false;for(auto& slot:slots_)if(!slot.live){free=true;break;}
 if(!free)return "{\"result\":\"WOULD_BLOCK\"}";
 auto& slot=slots_[next_%kSlots];next_+=1;
 slot.id=sequence_++;slot.token=token;slot.createdNs=now;slot.live=true;slot.mode=mode;slot.k=profile->k;slot.kw=profile->kw;
 char out[224];
 const auto written=std::snprintf(out,sizeof(out),"{\"result\":\"REQUESTED\",\"id\":%llu,\"schema\":\"%s\",\"page\":\"%s\",\"mode\":%s,\"prefixBytes\":65536,\"inputBytes\":4096,\"descriptorsMax\":32}",
  (unsigned long long)slot.id,kColdSchema,kColdPage,mode==0?"ROOT":mode==1?"FULL_CORE":"FULL_TRIALS");
 if(written<=0)return "{\"result\":\"INTERNAL\"}";
 return std::string(out,size_t(written));
}
std::string Cold::Read(uint64_t id,uint32_t segment,uint32_t wordOffset,uint64_t now){
 const Slot* found=nullptr;
 for(const auto& slot:slots_)if(slot.live&&slot.id==id){found=&slot;break;}
 if(!found)return "{\"result\":\"EXPIRED\"}";
 if(now-found->createdNs>=kTtlNs)return "{\"result\":\"EXPIRED\"}";
 // Segment dictionary: 0..4 water state tail, 5..18 wet WORK, 19+ parent.
 if(segment>=kDescriptorsMax)return "{\"result\":\"INVALID_SEGMENT\"}";
 PageJson page;
 page.Raw("{\"result\":\"PAGE\",\"id\":",21);page.Number(uint32_t(id));
 page.Raw(",\"segment\":",11);page.Number(segment);
 page.Raw(",\"offset\":",10);page.Number(wordOffset);
 page.Raw(",\"words\":",9);
 // The page window covers up to 1024 words of the requested segment.
 page.Number(kPageWords);
 page.Raw(",\"schema\":\"",12);page.Raw(kColdSchema,20);page.Raw("\",\"page\":\"",10);page.Raw(kColdPage,24);page.Raw("\"}",2);
 if(page.overflow)return "{\"result\":\"PAGE_OVERFLOW\"}";
 return std::string(page.data,page.length);
}
std::string Cold::End(uint64_t id,uint64_t now){
 for(auto& slot:slots_)if(slot.live&&slot.id==id){slot.live=false;return "{\"result\":\"ENDED\"}";}
 return "{\"result\":\"EXPIRED\"}";
}
}
