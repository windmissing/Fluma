// Caller supplies W[], m2LoadSand(slot,row) for candidate or spare, and two
// disjoint pad-word WORK ranges. All 64 invocations must enter these routines.
shared uint m2Ranks[64];
shared uint m2Deleted0, m2Deleted1, m2Inserted0, m2Inserted1;
shared uint m2Changed0, m2Changed1;

bool m2KeyLess(uint a, uint b, uint k) {
    uint ar[16], br[16];
    bool av = false, bv = false;
    if (a < k) { m2LoadSand(a, ar); av = m2Eligible(ar); }
    if (b < k) { m2LoadSand(b, br); bv = m2Eligible(br); }
    if (av != bv) return av;
    if (!av) return a < b;
    if (ar[7] != br[7]) return ar[7] < br[7];  // sourceId
    if (ar[9] != br[9]) return ar[9] < br[9];  // originHi
    if (ar[8] != br[8]) return ar[8] < br[8];  // originLo
    if (ar[6] != br[6]) return ar[6] < br[6];  // tag
    if (ar[14] != br[14]) return ar[14] < br[14];  // medium
    if ((ar[1] >> 20u) != (br[1] >> 20u)) return (ar[1] >> 20u) < (br[1] >> 20u);
    if (ar[2] != br[2]) return ar[2] < br[2];  // y
    return ar[5] != br[5] ? ar[5] < br[5] : a < b;
}

void m2IndexSort(uint at, uint pad, uint k) {
    uint lane = gl_LocalInvocationIndex;
    for (uint rank = lane; rank < pad; rank += 64u) W[at+rank] = rank;
    memoryBarrierBuffer(); barrier();
    for (uint len = 2u; len <= pad; len <<= 1u) {
        for (uint step = len >> 1u; step > 0u; step >>= 1u) {
            for (uint i = lane; i < pad; i += 64u) {
                uint j = i ^ step;
                if (j <= i) continue;
                uint a = W[at+i], b = W[at+j];
                bool swap = (i & len) == 0u ? m2KeyLess(b,a,k) : m2KeyLess(a,b,k);
                if (swap) { W[at+i] = b; W[at+j] = a; }
            }
            memoryBarrierBuffer(); barrier();
        }
    }
}

uint m2SlotId(uint slot) {
    uint row[16]; m2LoadSand(slot,row); return row[M2_sand16_id];
}
uint m2LowerId(uint a, uint b, uint k) {
    if (a >= k) return b;
    if (b >= k) return a;
    uint aId=m2SlotId(a), bId=m2SlotId(b);
    return (aId < bId || (aId == bId && a < b)) ? a : b;
}
uint m2ReduceId(uint slot, uint k) {
    uint lane=gl_LocalInvocationIndex;
    m2Ranks[lane]=slot; barrier();
    for (uint stride=32u; stride>0u; stride >>= 1u) {
        if (lane < stride) m2Ranks[lane]=m2LowerId(m2Ranks[lane],m2Ranks[lane+stride],k);
        barrier();
    }
    uint result=m2Ranks[0]; barrier();
    return result;
}
bool m2Witness(uint at, uint rank, uint k, uint a[16], uint radius) {
    uint slot=W[at+rank];
    if (slot>=k) return false;
    uint b[16]; m2LoadSand(slot,b);
    return m2Pair(a,b,radius)||m2Pair(b,a,radius);
}

uvec2 m2IndexPair(uint at, uint pad, uint k, uint radius) {
    uint lane=gl_LocalInvocationIndex, candidate=k;
    for (uint i=lane; i<pad; i+=64u) {
        uint slot=W[at+i];
        if (slot>=k) continue;
        uint a[16];m2LoadSand(slot,a);
        if (!m2Eligible(a)) continue;
        if ((i>0u && m2Witness(at,i-1u,k,a,radius)) || (i+1u<pad && m2Witness(at,i+1u,k,a,radius)))
            candidate=m2LowerId(candidate,slot,k);
    }
    uint first=m2ReduceId(candidate,k);
    if (first>=k) return uvec2(k);
    uint a[16];m2LoadSand(first,a);candidate=k;
    for (uint i=lane; i<pad; i+=64u) {
        uint slot=W[at+i];
        if (slot>=k) continue;
        uint b[16];m2LoadSand(slot,b);
        if (m2Pair(a,b,radius)) candidate=m2LowerId(candidate,slot,k);
    }
    return uvec2(first,m2ReduceId(candidate,k));
}

uint m2OldRank(uint rank, uint d0, uint d1) {
    if (rank>=d0) ++rank;
    if (rank>=d1) ++rank;
    return rank;
}
uint m2LowerBound(uint at, uint count, uint k, uint slot, uint d0, uint d1) {
    uint low=0u, high=count;
    while (low<high) {
        uint mid=(low+high)/2u;
        if (m2KeyLess(W[at+m2OldRank(mid,d0,d1)],slot,k)) low=mid+1u;
        else high=mid;
    }
    return low;
}

// Slot values have already changed, but the old index still contains their
// previous ranks. Delete by slot number before comparing any current keys.
void m2IndexUpdate(uint oldAt, uint newAt, uint pad, uint k, uint a, uint b) {
    uint lane=gl_LocalInvocationIndex;
    if (lane==0u) {
        uint ra=pad, rb=pad;
        for (uint i=0u; i<pad; ++i) {
            if (W[oldAt+i]==a) ra=i;
            if (b<k && W[oldAt+i]==b) rb=i;
        }
        m2Deleted0=min(ra,rb);m2Deleted1=max(ra,rb);
        uint removed=b<k?2u:1u;
        if (b<k && m2KeyLess(b,a,k)) {m2Changed0=b;m2Changed1=a;}
        else {m2Changed0=a;m2Changed1=b;}
        m2Inserted0=m2LowerBound(oldAt,pad-removed,k,m2Changed0,m2Deleted0,m2Deleted1);
        m2Inserted1=pad;
        if (b<k) m2Inserted1=1u+m2LowerBound(oldAt,pad-removed,k,m2Changed1,m2Deleted0,m2Deleted1);
    }
    barrier();
    for (uint i=lane; i<pad; i+=64u) {
        if (i==m2Inserted0) W[newAt+i]=m2Changed0;
        else if (i==m2Inserted1) W[newAt+i]=m2Changed1;
        else {
            uint r=i-uint(i>m2Inserted0)-uint(i>m2Inserted1);
            W[newAt+i]=W[oldAt+m2OldRank(r,m2Deleted0,m2Deleted1)];
        }
    }
    memoryBarrierBuffer(); barrier();
}
