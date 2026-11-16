#pragma once
#include "../control.h"
#include <GLES3/gl31.h>
namespace grs::m3b {
// Explicit V5-only bridge. Legacy lease data is an internal drawing descriptor;
// version/identity checks are performed before constructing it from a V5 token.
class SceneSourceV5 {
public:
 virtual ~SceneSourceV5()=default;
 virtual Words256 SceneHeaderV5()const=0;
 virtual ControlCode AcquireV5(uint32_t,U64,PublicationLease&,GLuint&)=0;
 virtual GLuint StateBufferV5(const PublicationLease&)const=0;
 virtual ControlCode ReleaseV5(uint32_t,U64,GLsync)=0;
};
}
