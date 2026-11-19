#include "controller.h"
namespace grs::g9rx {
extern "C" __attribute__((visibility("default")))
const uint32_t GrsG9RxAbi[] = {1,
                               sizeof(RxProtocolPayload),
                               alignof(RxProtocolPayload),
                               sizeof(RxSync),
                               alignof(RxSync),
                               sizeof(Controller),
                               sizeof(Transport),
                               sizeof(FrozenExpectedView),
                               sizeof(ProgressClockV3),
                               offsetof(RxProtocolPayload, witnesses),
                               offsetof(RxProtocolPayload, metadata),
                               offsetof(RxProtocolPayload, scratch),
                               offsetof(RxProtocolPayload, journal),
                               offsetof(RxProtocolPayload, control),
                               offsetof(RxProtocolPayload, inputs),
                               offsetof(RxProtocolPayload, bridge)};
}
