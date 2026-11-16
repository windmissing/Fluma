#pragma once
#include "../control.h"
#include <GLES3/gl31.h>
namespace grs::m3a {
// Explicit V4-only bridge. Legacy lease data is an internal drawing descriptor;
// version/identity checks are performed before constructing it from a V4 token.
class SceneSourceV4 {
public:
 virtual ~SceneSourceV4()=default;
 virtual Words256 SceneHeaderV4()const=0;
 virtual ControlCode AcquireV4(uint32_t,U64,PublicationLease&,GLuint&)=0;
 virtual ControlCode ReleaseV4(uint32_t,U64,GLsync)=0;
};
}
