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
    return api.Res && api.BuilderDone && api.ReadBegin && api.ReadEnd && api.Freed && api.GpuRes;
}

int main(int argc, char** argv) {
    const char* brokerDir = argc > 1 ? argv[1] : "./";
    g_log.open(std::string(brokerDir) + "/t636-driver-log.ndjson", std::ios::app);
    emit("START", "T636 cross-DSO broker harness");
    Api A, B;
    if (!load(A, (std::string(brokerDir) + "/libadapter_a.so").c_str())) return 1;
    if (!load(B, (std::string(brokerDir) + "/libadapter_b.so").c_str())) return 1;

    // Matrix 1: A Reserve succeeds; B Reserve (empty slots, enough budget) must
    // be rejected (single-builder gate) — then B BuilderDone/ReadBegin on A's
    // ticket must SUCCEED (cross-DSO ticket semantics).
    uint64_t tA = 0, req = 0, trace = 0;
    bool okA = A.Res(&tA, 4096, 8192, 1, &req, &trace);
    char b[160]; snprintf(b, sizeof(b), "A.Reserve(4096,route1)=%d ticket=%llu", (int)okA, (unsigned long long)tA);
    emit("M1_A_RESERVE", b);
    if (!okA) { emit("M1_ABORT", "A reserve failed"); return 2; }
    uint64_t tB = 0;
    bool okBsame = B.Res(&tB, 4096, 8192, 1, &req, &trace);
    snprintf(b, sizeof(b), "B.Reserve(4096,route1)=%d (expect 0: single builder)", (int)okBsame);
    emit("M1_B_RESERVE_SAME_ROUTE", b);
    okBsame = B.Res(&tB, 4096, 8192, 2, &req, &trace);
    snprintf(b, sizeof(b), "B.Reserve(4096,route2)=%d (expect 0: single builder global)", (int)okBsame);
    emit("M1_B_RESERVE_OTHER_ROUTE", b);
    B.BuilderDone(tA);   // B-prefix wrapper on A's ticket
    bool rb = B.ReadBegin(tA);
    snprintf(b, sizeof(b), "B.ReadBegin(A.ticket=%llu)=%d (expect 1: cross-DSO)", (unsigned long long)tA, (int)rb);
    emit("M1_B_READBEGIN_A_TICKET", b);

    // Matrix 2: reader A holds; B Reserve other route OK (builder/reader across
    // slots coexist); B builder-phase ReadBegin(B-ticket) rejected.
    uint64_t tB2 = 0;
    bool okB2 = B.Res(&tB2, 4096, 8192, 2, &req, &trace);
    snprintf(b, sizeof(b), "B.Reserve(4096,route2,readerA-active)=%d (expect 1)", (int)okB2);
    emit("M2_B_RESERVE_ROUTE2", b);
    if (okB2) {
        bool selfRead = B.ReadBegin(tB2);
        snprintf(b, sizeof(b), "B.ReadBegin(own builder ticket)=%d (expect 0)", (int)selfRead);
        emit("M2_B_SELF_READ", b);
        B.BuilderDone(tB2);
        bool afterDone = B.ReadBegin(tB2);
        snprintf(b, sizeof(b), "B.ReadBegin(B ticket after own BuilderDone)=%d (expect 0 until A reader frees)", (int)afterDone);
        emit("M2_B_READ_AFTER_DONE", b);
    }

    // Matrix 3: B Freed on A's ticket while A reader holds -> third Reserve
    // still rejected; A ReadEnd then re-ReadBegin OK (ticket alive); then B Freed.
    B.Freed(tA);
    uint64_t tX = 0;
    bool okX = A.Res(&tX, 4096, 8192, 1, &req, &trace);
    snprintf(b, sizeof(b), "A.Reserve after B.Freed(A.reader-active)=%d (expect 0)", (int)okX);
    emit("M3_FREED_WHILE_READER", b);
    A.ReadEnd(tA);
    bool reRead = A.ReadBegin(tA);
    snprintf(b, sizeof(b), "A.ReadEnd+ReadBegin(A ticket)=%d (expect 1)", (int)reRead);
    emit("M3_A_REREAD", b);
    A.ReadEnd(tA);
    B.Freed(tA);
    okX = A.Res(&tX, 4096, 8192, 1, &req, &trace);
    snprintf(b, sizeof(b), "A.Reserve after full release=%d (expect 1, new generation)", (int)okX);
    emit("M3_RESERVE_NEW_GEN", b);
    if (okX) { A.Freed(tX); }

    // Matrix 5 (GPU domain): A opaque-context reserve bytes=1; B second reserve
    // same ctx rejected; wrong-ticket Freed/wrong-ctx Retired don't release;
    // B Freed(A ticket) then B OK; B ContextRetired then A OK.
    uint64_t ctxA = 0xABCD0001ULL, gA = 0, gB = 0;
    bool g1 = A.GpuRes(&gA, ctxA, 1);
    snprintf(b, sizeof(b), "A.GpuReserve(ctxA,1B)=%d ticket=%llu", (int)g1, (unsigned long long)gA);
    emit("M5_A_GPU_RESERVE", b);
    if (g1) {
        bool g2 = B.GpuRes(&gB, ctxA, 1);
        snprintf(b, sizeof(b), "B.GpuReserve(same ctxA)=%d (expect 0)", (int)g2);
        emit("M5_B_GPU_SAME_CTX", b);
        B.GpuFreed(gA + 999);            // wrong ticket — must not release
        B.GpuCtxRetired(ctxA + 1);       // wrong context — must not release
        g2 = B.GpuRes(&gB, ctxA, 1);
        snprintf(b, sizeof(b), "B.GpuReserve after wrong-free/retire=%d (expect 0)", (int)g2);
        emit("M5_WRONG_FREE", b);
        B.GpuFreed(gA);                  // B frees A's ticket (cross-DSO)
        g2 = B.GpuRes(&gB, ctxA, 1);
        snprintf(b, sizeof(b), "B.GpuReserve after B.Freed(A ticket)=%d (expect 1)", (int)g2);
        emit("M5_B_FREED_A", b);
        if (g2) B.GpuFreed(gB);
        B.GpuCtxRetired(ctxA);
        bool g3 = A.GpuRes(&gA, ctxA, 1);
        snprintf(b, sizeof(b), "A.GpuReserve after B.ContextRetired=%d (expect 1)", (int)g3);
        emit("M5_CTX_RETIRED", b);
        if (g3) A.GpuFreed(gA);
    }
    emit("DONE", "matrix complete");
    return 0;
}
