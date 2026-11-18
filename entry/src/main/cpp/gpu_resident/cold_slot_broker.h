#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
// Implemented once in libgrs_cold_slots.so. Every route/DSO uses this API;
// neither inline statics nor symbol interposition provide process identity.
extern "C" {
__attribute__((visibility("default"))) bool GrsColdReserve(uint64_t* ticket,size_t bytes,size_t routeBudget,uint32_t route,uint64_t* request,uint64_t* trace);
__attribute__((visibility("default"))) void GrsColdBuilderDone(uint64_t ticket);
__attribute__((visibility("default"))) bool GrsColdReadBegin(uint64_t ticket);
__attribute__((visibility("default"))) void GrsColdReadEnd(uint64_t ticket);
__attribute__((visibility("default"))) void GrsColdFreed(uint64_t ticket);
__attribute__((visibility("default"))) bool GrsFullGpuReserve(uint64_t* ticket,uint64_t context,size_t bytes);
__attribute__((visibility("default"))) void GrsFullGpuFreed(uint64_t ticket);
__attribute__((visibility("default"))) void GrsFullGpuContextRetired(uint64_t context);

}
namespace grs::coldbroker {
// A forgotten/lost context never returns credit from a C++ destructor.
// ReleaseAfterFenceDelete acknowledges actual owner retirement; otherwise the
// broker retains the ticket until eglDestroyContext succeeds.
class GpuLease {
 uint64_t ticket_=0;
public:
 GpuLease()=default;GpuLease(const GpuLease&)=delete;GpuLease& operator=(const GpuLease&)=delete;
 bool Reserve(uint64_t context,size_t bytes){return ticket_||GrsFullGpuReserve(&ticket_,context,bytes);}
 void ReleaseAfterFenceDelete(){if(ticket_){GrsFullGpuFreed(ticket_);ticket_=0;}}
 void ContextLost(){ticket_=0;} // process broker keeps the real retiring credit
 bool Held()const{return ticket_!=0;}
};
class Lease {
 uint64_t ticket_=0;
public:
 Lease()=default;Lease(const Lease&)=delete;Lease& operator=(const Lease&)=delete;
 Lease(Lease&& b)noexcept:ticket_(std::exchange(b.ticket_,0)){}
 Lease& operator=(Lease&& b)noexcept{if(this!=&b){Reset();ticket_=std::exchange(b.ticket_,0);}return *this;}
 ~Lease(){Reset();}
 bool Reserve(size_t bytes,size_t budget,uint32_t route,uint64_t& request,uint64_t& trace){return GrsColdReserve(&ticket_,bytes,budget,route,&request,&trace);}
 void BuilderDone(){if(ticket_)GrsColdBuilderDone(ticket_);}
 bool ReadBegin(){return ticket_&&GrsColdReadBegin(ticket_);}
 void ReadEnd(){if(ticket_)GrsColdReadEnd(ticket_);}
 void Reset(){if(ticket_){GrsColdFreed(ticket_);ticket_=0;}}
};
// Logical Trim cannot refund a live detached allocation. The token follows
// the pointer and is returned only after delete[], including during unwind.
class DetachedBlock {
 Lease credit_;std::unique_ptr<uint32_t[]> block_;
public:
 DetachedBlock()=default;
 DetachedBlock(Lease&& credit,std::unique_ptr<uint32_t[]>&& block):credit_(std::move(credit)),block_(std::move(block)){}
 DetachedBlock(const DetachedBlock&)=delete;DetachedBlock& operator=(const DetachedBlock&)=delete;
 DetachedBlock(DetachedBlock&&)=default;
 DetachedBlock& operator=(DetachedBlock&& b)noexcept{if(this!=&b){reset();credit_=std::move(b.credit_);block_=std::move(b.block_);}return *this;}
 void reset(){block_.reset();credit_.Reset();}
 uint32_t* get()const{return block_.get();}explicit operator bool()const{return bool(block_);}
};
}
