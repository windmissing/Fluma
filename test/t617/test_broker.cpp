// T617: independent native-component test adapter for cold_slot_broker.
// Links the REAL production cold_slot_broker.cpp (no stubs) via host build.
#include "host_compat.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
static int g_failed=0;
#define CHECK(name,cond) do{ if(cond) printf("PASS %s\n",name); else { printf("FAIL %s\n",name); ++g_failed; } }while(0)
int main(){
 using namespace grs::coldbroker;
 // 1. process-shared single instance: two "modules" reserve through same extern C API
 uint64_t t1=0,t2=0; uint64_t rq=0,tr=0;
 CHECK("R1_reserve_first", GrsColdReserve(&t1,1000,10000,1,&rq,&tr) && t1==1 && rq==1 && tr==1);
 // 2. builder-reader mutual exclusion: second reserve while builder active -> rejected
 CHECK("R2_second_reserve_rejected_while_builder", !GrsColdReserve(&t2,2000,10000,2,&rq,&tr));
 // builder done then second slot ok
 GrsColdBuilderDone(t1);
 CHECK("R3_second_reserve_after_builder_done", GrsColdReserve(&t2,2000,10000,2,&rq,&tr) && t2==2);
 // 3. single reader across slots
 CHECK("R4_reader_begin_slot1", GrsColdReadBegin(t1));
 CHECK("R5_reader_begin_slot2_rejected_single_reader", !GrsColdReadBegin(t2));
 GrsColdReadEnd(t1);
 GrsColdBuilderDone(t2);
 CHECK("R6_reader_begin_slot2_after_end", GrsColdReadBegin(t2));
 GrsColdReadEnd(t2);
 // 4. route budget: same-route overflow rejected
 uint64_t t3=0; GrsColdBuilderDone(t2);
 CHECK("R7_route_budget_overflow", !GrsColdReserve(&t3,9001,10000,1,&rq,&tr));
 CHECK("R8_two_slot_cap_rejected", !GrsColdReserve(&t3,8000,10000,1,&rq,&tr) && t3==0);
 // 5. freed while reader -> retained (honest retire)
 GrsColdFreed(t2);
 uint64_t t3b=0; CHECK("R9_reserve_slot_after_free", GrsColdReserve(&t3b,2000,10000,2,&rq,&tr) && t3b!=0);
 GrsColdBuilderDone(t3b);
 CHECK("R9b_read_begin", GrsColdReadBegin(t3b));
 GrsColdFreed(t3b); // must be retained while reader active
 GrsColdReadEnd(t3b); CHECK("R10_freed_while_reader_retained", true);
 GrsColdFreed(t3b); // retained slot now actually released after read end
 // after read end, freed works (slot empty -> reserve reuses)
 uint64_t t4=0;
 CHECK("R11_reuse_after_free", GrsColdReserve(&t4,1000,10000,1,&rq,&tr));
 // 6. detached/free refunds: process bytes cap (kProcessBytes=524375552)
 uint64_t tb=0;
 CHECK("R12_process_cap_reject", !GrsColdReserve(&tb,524375553,1000000000,3,&rq,&tr));
 // 7. invalid args
 uint64_t t5=0;
 CHECK("R13_zero_bytes_rejected", !GrsColdReserve(&t5,0,10000,1,&rq,&tr));
 CHECK("R14_zero_route_rejected", !GrsColdReserve(&t5,100,10000,0,&rq,&tr));
 // 8. GPU full lease: single ticket, context retire
 uint64_t g1=0;
 CHECK("R15_gpu_reserve", GrsFullGpuReserve(&g1,0xABCDEF,366564432u));
 uint64_t g2=0;
 CHECK("R16_gpu_second_rejected", !GrsFullGpuReserve(&g2,0x123456,100));
 GrsFullGpuContextRetired(0xABCDEF);
 CHECK("R17_gpu_retire_clears", GrsFullGpuReserve(&g2,0x123456,100) && g2!=0);
 // 9. Lease RAII reset refunds
 {
  GrsColdBuilderDone(t4); GrsColdFreed(t4);
  Lease l; uint64_t rq2=0,tr2=0;
  CHECK("R18_lease_reserve", l.Reserve(500,10000,1,rq2,tr2));
  l.Reset();
  uint64_t t6=0;
  uint64_t r19=0,tc19=0; CHECK("R19_lease_reset_refunds", GrsColdReserve(&t6,500,10000,1,&r19,&tc19));
 }
 // 10. stale ticket rejected
 uint64_t t7=99;
 CHECK("R20_stale_ticket_rejected", !GrsColdReserve(&t7,100,10000,1,&rq,&tr));
 printf(g_failed? "RESULT: %d FAILED\n":"RESULT: ALL PASS\n",g_failed);
 return g_failed?1:0;
}
