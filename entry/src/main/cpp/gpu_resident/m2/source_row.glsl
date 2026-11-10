// SOURCE128 format operations shared by the writer and the independent replay
// cut. Caller owns a distinct binding 3 SOURCE128 arena and the root cursor.
uvec4 m2SourceMask(uint kind) {
    if (kind == M2_MERGE) return uvec4(M2_MERGE_mask0,M2_MERGE_mask1,M2_MERGE_mask2,M2_MERGE_mask3);
    if (kind == M2_BIRTH) return uvec4(M2_BIRTH_mask0,M2_BIRTH_mask1,M2_BIRTH_mask2,M2_BIRTH_mask3);
    if (kind == M2_DIRECT) return uvec4(M2_DIRECT_mask0,M2_DIRECT_mask1,M2_DIRECT_mask2,M2_DIRECT_mask3);
    if (kind == M2_END) return uvec4(M2_END_mask0,M2_END_mask1,M2_END_mask2,M2_END_mask3);
    return uvec4(0u);
}

bool m2SourceBodyValid(uint row[128], uint ordinal) {
    uint kind=row[M2_sourceEvent128_kind];
    if (kind < M2_MERGE || kind > M2_END || ordinal == 0u ||
        row[M2_sourceEvent128_magic] != M2_sourceMagic || row[M2_sourceEvent128_revision] != M2_rev_wire ||
        row[M2_sourceEvent128_rootKind] != 1u || row[M2_sourceEvent128_ordinalLo] != ordinal ||
        row[M2_sourceEvent128_ordinalHi] != 0u || (row[M2_sourceEvent128_flags] & ~7u) != 0u) return false;
    uvec4 mask=m2SourceMask(kind);
    for (uint w=0u;w<128u;++w) {
        if ((mask[w/32u] & (1u << (w%32u))) == 0u && row[w] != 0u) return false;
    }
    return true;
}

// Single writer. A caller may advance its committed cursor only after true.
// Rows preceding the cursor are immutable. A failed body never changes E.
bool m2SealSourceRow(uint row[128], uint cursor, uint capacity) {
    if (capacity > 1025u || cursor >= capacity || row[M2_sourceEvent128_rowSeal] != 0u || !m2SourceBodyValid(row,cursor+1u)) return false;
    uint offset=cursor*128u;
    E[offset+M2_sourceEvent128_rowSeal]=0u;
    for (uint w=0u;w<128u;++w) {
        if (w!=M2_sourceEvent128_rowSeal) E[offset+w]=row[w];
    }
    memoryBarrierBuffer();
    E[offset+M2_sourceEvent128_rowSeal]=M2_sourceSeal;
    memoryBarrierBuffer();
    return true;
}

bool m2ReadSourceRow(uint index, uint sealedRows, uint capacity, out uint row[128]) {
    if (capacity > 1025u || index >= sealedRows || sealedRows > capacity) return false;
    uint offset=index*128u;
    if (E[offset+M2_sourceEvent128_rowSeal] != M2_sourceSeal) return false;
    for (uint w=0u;w<128u;++w) row[w]=E[offset+w];
    return m2SourceBodyValid(row,index+1u);
}
