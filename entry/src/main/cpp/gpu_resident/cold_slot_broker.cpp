#include "cold_slot_broker.h"
#include <array>
#include <mutex>
#include <limits>
namespace {
struct Slot {uint64_t ticket=0;size_t bytes=0;uint32_t route=0;bool builder=false,reader=false;};
std::mutex mutex;std::array<Slot,2> slots{};uint64_t serial=0,requests=0;
constexpr size_t kProcessBytes=524375552;
Slot* Find(uint64_t ticket){for(auto& slot:slots)if(ticket&&slot.ticket==ticket)return &slot;return nullptr;}
}
extern "C" bool GrsColdReserve(uint64_t* ticket,size_t bytes,size_t routeBudget,uint32_t route,uint64_t* request,uint64_t* trace){
 if(!ticket||!request||!trace||!bytes||!route||bytes>routeBudget||routeBudget>kProcessBytes)return false;
 std::lock_guard<std::mutex> lock(mutex);Slot* target=Find(*ticket);if(*ticket&&!target)return false;
 for(const auto& slot:slots)if(slot.builder)return false;
 if(target&&(target->reader||target->route!=route))return false;
 if(!target){for(auto& slot:slots)if(!slot.ticket){target=&slot;break;}}
 if(!target||requests==UINT64_MAX||(!*ticket&&serial==UINT64_MAX))return false;
 size_t total=bytes,local=bytes;for(const auto& slot:slots)if(&slot!=target){if(slot.bytes>kProcessBytes-total)return false;total+=slot.bytes;if(slot.route==route){if(slot.bytes>routeBudget-local)return false;local+=slot.bytes;}}
 if(total>kProcessBytes||local>routeBudget)return false;
 if(!*ticket)*ticket=++serial;target->ticket=*ticket;target->bytes=bytes;target->route=route;target->builder=true;
 *request=*trace=++requests;return true;
}
extern "C" void GrsColdBuilderDone(uint64_t ticket){std::lock_guard<std::mutex> lock(mutex);if(auto* slot=Find(ticket))slot->builder=false;}
extern "C" bool GrsColdReadBegin(uint64_t ticket){std::lock_guard<std::mutex> lock(mutex);auto* slot=Find(ticket);if(!slot||slot->builder)return false;for(const auto& other:slots)if(other.reader)return false;slot->reader=true;return true;}
extern "C" void GrsColdReadEnd(uint64_t ticket){std::lock_guard<std::mutex> lock(mutex);if(auto* slot=Find(ticket))slot->reader=false;}
extern "C" void GrsColdFreed(uint64_t ticket){std::lock_guard<std::mutex> lock(mutex);if(auto* slot=Find(ticket)){if(slot->reader)return;*slot={};}}
