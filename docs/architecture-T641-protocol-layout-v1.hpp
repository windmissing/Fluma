// Engineering ABI specimen only. No product logic, storage instance or field evolution.
#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
namespace t641_abi {
using U32=uint32_t;
struct alignas(64) RxProtocolPayload {std::array<U32,10240> words;};
// Alternative typed description of the SAME payload, never a second allocation.
struct alignas(64) RxTypedStorage {
 std::array<std::array<U32,256>,20> witnesses;
 std::array<std::array<U32,64>,4> metadata;
 std::array<U32,256> scratch;
 std::array<std::array<U32,16>,16> journal;
 std::array<U32,256> control;
 std::array<std::array<U32,128>,19> inputs;
 std::array<U32,1664> bridge;
};
struct RxSyncState {
 std::mutex protocol;
 std::atomic_flag ingress=ATOMIC_FLAG_INIT;
 std::atomic<uint64_t> strokeCancelGeneration{0};
 std::atomic<uint64_t> lifecycleInvalidation{0};
};
static_assert(sizeof(U32)==4);
static_assert(sizeof(RxProtocolPayload)==40960 && alignof(RxProtocolPayload)==64);
static_assert(sizeof(RxTypedStorage)==40960 && alignof(RxTypedStorage)==64);
static_assert(offsetof(RxTypedStorage,witnesses)==0);
static_assert(offsetof(RxTypedStorage,metadata)==20480);
static_assert(offsetof(RxTypedStorage,scratch)==21504);
static_assert(offsetof(RxTypedStorage,journal)==22528);
static_assert(offsetof(RxTypedStorage,control)==23552);
static_assert(offsetof(RxTypedStorage,inputs)==24576);
static_assert(offsetof(RxTypedStorage,bridge)==34304);
static_assert(sizeof(RxSyncState)<=256);
}
