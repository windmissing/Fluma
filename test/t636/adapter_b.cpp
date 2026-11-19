// T636 adapter A: independent DSO linking the production libgrs_cold_slots.so
// via DT_NEEDED; exposes B-prefixed wrappers that operate on A-obtained tickets
// (and vice versa) per architecture-T636-v1.md.
#include "../../entry/src/main/cpp/gpu_resident/cold_slot_broker.h"

extern "C" {

// A-side direct calls (thin, preserve raw bool + out-params).
__attribute__((visibility("default"))) bool A_Reserve(uint64_t* ticket, size_t bytes, size_t routeBudget,
                                                      uint32_t route, uint64_t* request, uint64_t* trace) {
    return GrsColdReserve(ticket, bytes, routeBudget, route, request, trace);
}
__attribute__((visibility("default"))) void A_BuilderDone(uint64_t t) { GrsColdBuilderDone(t); }
__attribute__((visibility("default"))) bool A_ReadBegin(uint64_t t) { return GrsColdReadBegin(t); }
__attribute__((visibility("default"))) void A_ReadEnd(uint64_t t) { GrsColdReadEnd(t); }
__attribute__((visibility("default"))) void A_Freed(uint64_t t) { GrsColdFreed(t); }
__attribute__((visibility("default"))) bool A_GpuReserve(uint64_t* ticket, uint64_t ctx, size_t bytes) {
    return GrsFullGpuReserve(ticket, ctx, bytes);
}
__attribute__((visibility("default"))) void A_GpuFreed(uint64_t t) { GrsFullGpuFreed(t); }
__attribute__((visibility("default"))) void A_GpuContextRetired(uint64_t ctx) { GrsFullGpuContextRetired(ctx); }

// B-prefixed wrappers operating on A-obtained tickets (same broker instance —
// cross-DSO ticket semantics is exactly what this harness proves or refutes).
__attribute__((visibility("default"))) bool B_BuilderDone(uint64_t t) { return (GrsColdBuilderDone(t), true); }
__attribute__((visibility("default"))) bool B_ReadBegin(uint64_t t) { return GrsColdReadBegin(t); }
__attribute__((visibility("default"))) void B_ReadEnd(uint64_t t) { GrsColdReadEnd(t); }
__attribute__((visibility("default"))) void B_Freed(uint64_t t) { GrsColdFreed(t); }
__attribute__((visibility("default"))) bool B_Reserve(uint64_t* ticket, size_t bytes, size_t routeBudget,
                                                      uint32_t route, uint64_t* request, uint64_t* trace) {
    return GrsColdReserve(ticket, bytes, routeBudget, route, request, trace);
}
__attribute__((visibility("default"))) bool B_GpuReserve(uint64_t* ticket, uint64_t ctx, size_t bytes) {
    return GrsFullGpuReserve(ticket, ctx, bytes);
}
__attribute__((visibility("default"))) void B_GpuFreed(uint64_t t) { GrsFullGpuFreed(t); }
__attribute__((visibility("default"))) void B_GpuContextRetired(uint64_t ctx) { GrsFullGpuContextRetired(ctx); }

} // extern "C"
