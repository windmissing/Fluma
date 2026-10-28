#pragma once
#include <cstdint>
#include <cstddef>
namespace grs {
struct FoundationInfo {
 const char* numericPolicy;const char* timePolicy;const char* renderPolicy;const char* layoutSha;const char* angleSha;const char* primitiveSha;
 uint32_t implementedFeatures;bool stepShaderImplemented;bool connectedToXComponent;
};
FoundationInfo GetFoundationInfo();const uint32_t* ColdAngleWords(size_t& count);const char* PrimitiveLibrarySource();
}
