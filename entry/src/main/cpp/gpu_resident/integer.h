// T236 GRS_Q20_V1 integer primitives; not a CPU simulation backend.
#pragma once
#include <cstdint>
#include <cstddef>
namespace grs {
struct U64 {uint32_t lo=0,hi=0;};
struct U128 {uint32_t w[4]{};};
bool Add64(U64,U64,U64&);bool Sub64(U64,U64,U64&);int Compare64(U64,U64);
U64 Mul32(uint32_t,uint32_t);U128 Mul64(U64,U64);
int Compare128(U128,U128);
bool FloorDiv(int32_t,int32_t,int32_t&);bool CeilDiv(int32_t,int32_t,int32_t&);
bool Divide128(U128,uint32_t,U128&,uint32_t&);
bool MulDivRNE(int32_t,int32_t,uint32_t,int32_t&);
// Raw binary64 finite nonnegative floor(x*2^20). Upper bound is in units.
// Positive sub-quantum values return units=0 with valid=true. Admission must reject them.
bool FloorBinary64Q20(uint32_t lo,uint32_t hi,uint32_t maxUnits,uint32_t&);
// Software IEEE binary32 RNE for nonnegative integer * 2^-20.
uint32_t Q20FloatBits(uint32_t units);uint32_t ColumnPreservingFloatBits(uint32_t x);
bool CheckedBytes(size_t words,size_t maxBytes,size_t& bytes);
bool AlignUp(size_t bytes,size_t alignment,size_t maxBytes,size_t& result);
}
