// T636 driver: dlopens adapter_a and adapter_b DSOs (each DT_NEEDED the same
// production libgrs_cold_slots.so bytes), runs the architecture-T636 ordered
// matrix, logs every call with in/out values. Single process, <=30s, <=128 log
// lines x <=512B. Route A=1, B=2. No GL.
#include <dlfcn.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <fstream>

typedef bool (*ReserveFn)(uint64_t*, size_t, size_t, uint32_t, uint64_t*, uint64_t*);
typedef void (*VoidFn)(uint64_t);
typedef bool (*BoolFn)(uint64_t);
typedef bool (*GpuReserveFn)(uint64_t*, uint64_t, size_t);

static std::ofstream g_log;
static int g_seq = 0;
static void emit(const std::string& event, const std::string& detail) {
    if (!g_log.is_open() || g_seq >= 128) return;
    g_log << "{\"seq\":" << (g_seq++) << ",\"event\":\"" << event << "\",\"detail\":\"" << detail << "\"}\n";
}

struct Api {
    ReserveFn Res; VoidFn BuilderDone; BoolFn ReadBegin; VoidFn ReadEnd; VoidFn Freed;
    GpuReserveFn GpuRes; VoidFn GpuFreed; VoidFn GpuCtxRetired;
    void* h = nullptr;
};

static bool load(Api& api, const char* so) {
    api.h = dlopen(so, RTLD_NOW);
    if (!api.h) { emit("DLOPEN_FAIL", std::string(so) + " dlerror=" + dlerror()); return false; }
    api.Res = (ReserveFn)dlsym(api.h, "A_Reserve"); if (!api.Res) api.Res = (ReserveFn)dlsym(api.h, "B_Reserve");
    api.BuilderDone = (VoidFn)dlsym(api.h, "A_BuilderDone");
    api.ReadBegin = (BoolFn)dlsym(api.h, "A_ReadBegin");
    api.ReadEnd = (VoidFn)dlsym(api.h, "A_ReadEnd");
    api.Freed = (VoidFn)dlsym(api.h, "A_Freed");
    api.GpuRes = (GpuReserveFn)dlsym(api.h, "A_GpuReserve");
    api.GpuFreed = (VoidFn)dlsym(api.h, "A_GpuFreed");
    api.GpuCtxRetired = (VoidFn)dlsym(api.h, "A_GpuContextRetired");
    char buf[160]; snprintf(buf, sizeof(buf), "%s handle=%p Res=%p ReadBegin=%p", so, api.h, (void*)api.Res, (void*)api.ReadBegin);
    emit("LOADED", buf);
    bool ok = api.Res && api.BuilderDone && api.ReadBegin && api.ReadEnd && api.Freed
              && api.GpuRes && api.GpuFreed && api.GpuCtxRetired;
    char nm[64]; Dl_info inf{};
    if (ok && dladdr((void*)api.Res,&inf)) { snprintf(nm,sizeof(nm),"%s",inf.dli_sname?inf.dli_sname:"?"); }
    else snprintf(nm,sizeof(nm),"?");
    char buf2[96]; snprintf(buf2,sizeof(buf2),"resolved=%p name=%s",(void*)api.Res,nm);
    emit("RESOLVE", buf2);
    return ok;
}

int main(int argc, char** argv) {
    const char* brokerDir = argc > 1 ? argv[1] : "./";
    g_log.open(std::string(brokerDir) + "/t636-driver-log.ndjson", std::ios::app);
    emit("START", "T636 cross-DSO broker harness");
    Api A, B;
    if (!load(A, (std::string(brokerDir) + "/libadapter_a.so").c_str())) return 1;
    if (!load(B, (std::string(brokerDir) + "/libadapter_b.so").c_str())) return 1;

    // ===== T656 corrected matrix (fresh0 probes, full M4, single run) =====
    // M1: A reserve(fresh0) OK; B reserve rejects; B BuilderDone/ReadBegin on A ticket OK.
    uint64_t tA=0, req=0, trace=0;
    bool okA = A.Res(&tA, 4096, 8192, 1, &req, &trace);
    char b[192]; snprintf(b,sizeof(b),"A.Res(4096,r1)=%d ticket_before=0 ticket_after=%llu",(int)okA,(unsigned long long)tA);
    emit("M1_A_RES_FRESH0", b);
    if(!okA){emit("M1_ABORT","A reserve failed");return 2;}
    { uint64_t t=0; bool r=B.Res(&t,4096,8192,1,&req,&trace);
      snprintf(b,sizeof(b),"B.Res(r1,fresh0)=%d after=%llu",(int)r,(unsigned long long)t); emit("M1_B_RES_R1",b); }
    { uint64_t t=0; bool r=B.Res(&t,4096,8192,2,&req,&trace);
      snprintf(b,sizeof(b),"B.Res(r2,fresh0)=%d after=%llu",(int)r,(unsigned long long)t); emit("M1_B_RES_R2",b); }
    B.BuilderDone(tA);
    { bool r=B.ReadBegin(tA); snprintf(b,sizeof(b),"B.ReadBegin(A.ticket=%llu)=%d",(unsigned long long)tA,(int)r); emit("M1_B_READBEGIN_A",b);
      if(r){ B.ReadEnd(tA); emit("M1_B_READEND","ok"); } }
    A.Freed(tA); emit("M1_A_FREED","tA released");

    // M2: reader A active; B reserve other route OK; B self ReadBegin reject.
    { uint64_t tB2=0; bool r=B.Res(&tB2,4096,8192,2,&req,&trace);
      snprintf(b,sizeof(b),"B.Res(r2,readerA-active,fresh0)=%d after=%llu",(int)r,(unsigned long long)tB2); emit("M2_B_RES_R2",b);
      if(r){ bool s=B.ReadBegin(tB2); snprintf(b,sizeof(b),"B.ReadBegin(own builder)=%d (expect 0)",(int)s); emit("M2_B_SELF_READ",b);
        B.BuilderDone(tB2);
        bool a2=B.ReadBegin(tB2); snprintf(b,sizeof(b),"B.ReadBegin(own done, readerA active)=%d (expect 0)",(int)a2); emit("M2_B_READ_AFTER_DONE",b);
        // global reader gate isolation: free A's reader, then B ticket read OK.
        A.ReadEnd(tA);
        bool a3=B.ReadBegin(tB2); snprintf(b,sizeof(b),"B.ReadBegin(own done, readers freed)=%d (expect 1)",(int)a3); emit("M2_GLOBAL_READER_GATE",b);
        if(a3){ B.ReadEnd(tB2); B.Freed(tB2); emit("M2_B_CLEANUP","ok"); }
      } }

    // M3: Freed-while-reader ordering (fresh0 each probe).
    { uint64_t t1=0; bool r=A.Res(&t1,4096,8192,1,&req,&trace);
      snprintf(b,sizeof(b),"A.Res=%d ticket=%llu",(int)r,(unsigned long long)t1); emit("M3_A_RES",b);
      if(r){ A.ReadBegin(t1); B.Freed(t1);
        { uint64_t t=0; bool x=A.Res(&t,4096,8192,1,&req,&trace);
          snprintf(b,sizeof(b),"A.Res(after B.Freed, reader-active)=%d (expect 0)",(int)x); emit("M3_FREED_WHILE_READER",b); }
        A.ReadEnd(t1);
        { bool rr=A.ReadBegin(t1); snprintf(b,sizeof(b),"A.ReadEnd+ReadBegin=%d (expect 1)",(int)rr); emit("M3_REREAD",b); }
        A.ReadEnd(t1); A.Freed(t1);
        { uint64_t t2=0; bool x=A.Res(&t2,4096,8192,1,&req,&trace);
          snprintf(b,sizeof(b),"A.Res(fresh0, after release)=%d ticket=%llu (new gen)",(int)x,(unsigned long long)t2); emit("M3_NEW_GEN",b);
          if(x){ A.ReadBegin(t2); A.ReadEnd(t2); A.Freed(t2); emit("M3_CLEANUP","ok"); } } } }

    // M4: route/process accumulated credit (T636 contract, fresh0, one slot).
    { uint64_t tR=0; bool r=A.Res(&tR,4096,8192,1,&req,&trace);
      emit("M4_A_RES_R1", r?"ok":"FAIL");
      if(r){ A.BuilderDone(tR);
        { uint64_t t=0; bool x=A.Res(&t,4097,8192,1,&req,&trace);
          snprintf(b,sizeof(b),"A.Res(4097,r1 after 4096)=%d (expect 0: route total 8193>8192)",(int)x); emit("M4_ROUTE_4097_REJ",b); }
        { uint64_t t=0; bool x=A.Res(&t,4096,8192,1,&req,&trace);
          snprintf(b,sizeof(b),"A.Res(4096,r1)=%d (expect 0: route full)",(int)x); emit("M4_ROUTE_FULL",b); }
        A.Freed(tR);
        // different route, process cap: charge = 524375552-4096, budget same.
        { uint64_t t2=0; bool x=A.Res(&t2,524375552-4096,524375552,2,&req,&trace);
          snprintf(b,sizeof(b),"A.Res(charge=cap-4096,r2)=%d",(int)x); emit("M4_PROC_CHARGE",b);
          if(x){ A.BuilderDone(t2);
            { uint64_t t3=0; bool y=A.Res(&t3,4097,524375552,3,&req,&trace);
              snprintf(b,sizeof(b),"A.Res(4097,r3 during charge)=%d (expect 0: process cap)",(int)y); emit("M4_PROC_4097_REJ",b); }
            { uint64_t t3=0; bool y=A.Res(&t3,4096,524375552,3,&req,&trace);
              snprintf(b,sizeof(b),"A.Res(4096,r3)=%d (expect 1: within cap)",(int)y); emit("M4_PROC_4096_OK",b);
              if(y){ A.Freed(t3); } }
            A.Freed(t2); emit("M4_CLEANUP","ok"); } } } }

    // M5 GPU domain (fresh0 probes, T656 corrected order).
    { uint64_t ctxA=0xABCD0001ULL, gA=0;
      bool g1=A.GpuRes(&gA,ctxA,1);
      snprintf(b,sizeof(b),"A.GpuRes(ctxA,1B)=%d ticket_after=%llu",(int)g1,(unsigned long long)gA); emit("M5_A_GPU_RES",b);
      if(g1){
        { uint64_t g=0; bool x=B.GpuRes(&g,ctxA,1); snprintf(b,sizeof(b),"B.GpuRes(same ctx)=%d (expect 0)",(int)x); emit("M5_B_SAME_CTX",b); }
        { uint64_t g=0; B.GpuFreed(gA+999); B.GpuCtxRetired(ctxA+1);
          bool x=B.GpuRes(&g,ctxA,1); snprintf(b,sizeof(b),"B.GpuRes(after wrong-free/retire)=%d (expect 0)",(int)x); emit("M5_WRONG_GUARDS",b); }
        B.GpuFreed(gA);
        { uint64_t g=0; bool x=B.GpuRes(&g,ctxA,1); snprintf(b,sizeof(b),"B.GpuRes(after B.Freed(A ticket))=%d (expect 1)",(int)x); emit("M5_B_FREED_A",b);
          if(x){ uint64_t gKeep=g; // keep gB ACTIVE — corrected order
            B.GpuCtxRetired(ctxA);
            { uint64_t g2=0; bool y=A.GpuRes(&g2,ctxA,1); snprintf(b,sizeof(b),"A.GpuRes(fresh0, after B.ContextRetired(ctxA))=%d (expect 1: retired cleared lease)",(int)y); emit("M5_CTX_RETIRED_CORRECT",b);
              if(y){ A.GpuFreed(g2); } }
            B.GpuFreed(gKeep); } } }
    }
    emit("DONE","T656 corrected matrix complete");
    return 0;
}
