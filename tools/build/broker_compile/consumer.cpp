#include "cold_slot_broker.h"
// Compile/link references only. This library is never loaded or executed.
extern "C" __attribute__((visibility("default"))) void* GrsT666AbiReferences[]={reinterpret_cast<void*>(&GrsColdReserve),reinterpret_cast<void*>(&GrsColdBuilderDone),reinterpret_cast<void*>(&GrsColdReadBegin),reinterpret_cast<void*>(&GrsColdReadEnd),reinterpret_cast<void*>(&GrsColdFreed),reinterpret_cast<void*>(&GrsFullGpuReserve),reinterpret_cast<void*>(&GrsFullGpuFreed),reinterpret_cast<void*>(&GrsFullGpuContextRetired)};
