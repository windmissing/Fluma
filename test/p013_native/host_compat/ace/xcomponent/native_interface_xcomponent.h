#pragma once
// Host platform double only. The normal hvigor build checks the real SDK ABI.
#include <cstdint>
extern "C" {
struct OH_NativeXComponent;
int32_t OH_NativeXComponent_GetXComponentSize(OH_NativeXComponent*, const void*, uint64_t*, uint64_t*);
}
#define OH_NATIVE_XCOMPONENT_OBJ "__NATIVE_XCOMPONENT_OBJ__"
constexpr uint32_t OH_XCOMPONENT_ID_LEN_MAX = 128;
struct OH_NativeXComponent_Callback {
 void (*OnSurfaceCreated)(OH_NativeXComponent*,void*);
 void (*OnSurfaceChanged)(OH_NativeXComponent*,void*);
 void (*OnSurfaceDestroyed)(OH_NativeXComponent*,void*);
 void (*DispatchTouchEvent)(OH_NativeXComponent*,void*);
};
extern "C" {
int32_t OH_NativeXComponent_GetXComponentId(OH_NativeXComponent*,char*,uint64_t*);
int32_t OH_NativeXComponent_RegisterCallback(OH_NativeXComponent*,OH_NativeXComponent_Callback*);
}
