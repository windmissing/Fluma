// T617 host-only compat shim (tester test facility; product header untouched).
// Maps GCC visibility attribute away for MSVC host builds of the REAL broker.
#if defined(_MSC_VER) && !defined(GRS_HOST_NO_ATTR)
#define __attribute__(x)
#endif
#include "../../entry/src/main/cpp/gpu_resident/cold_slot_broker.h"
