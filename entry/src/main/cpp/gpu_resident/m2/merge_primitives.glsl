// T440 signed weighted nearest-even. Caller has audited the slot words.
bool m2WeightedRne(uint ma, int a, uint mb, int b, out int value) {
    value = 0;
    if (ma == 0u || mb == 0u || ma > M2_cfg_lightMax || mb > M2_cfg_lightMax ||
        a < -1073741824 || a > 1073741824 || b < -1073741824 || b > 1073741824) return false;
    uint den = ma + mb;
    uvec2 pa = grsMul32(ma, uint(abs(a)));
    uvec2 pb = grsMul32(mb, uint(abs(b)));
    bool negative;
    uvec2 magnitude;
    if ((a < 0) == (b < 0)) {
        if (!grsAdd64(pa, pb, magnitude)) return false;
        negative = a < 0;
    } else if (grsCompare64(pa, pb) >= 0) {
        if (!grsSub64(pa, pb, magnitude)) return false;
        negative = a < 0;
    } else {
        if (!grsSub64(pb, pa, magnitude)) return false;
        negative = b < 0;
    }
    // The audited magnitude is strictly below 2^48 and the divisor is below 2^18.
    uint q = 0u, r = 0u;
    for (int i = 47; i >= 0; --i) {
        uint bit = (magnitude[i / 32] >> uint(i % 32)) & 1u;
        r = (r << 1u) | bit;
        if (r >= den) {
            if (i >= 32) return false;
            q |= 1u << uint(i);
            r -= den;
        }
    }
    if (r > den - r || (r == den - r && (q & 1u) != 0u)) ++q;
    value = negative ? -int(q) : int(q);
    return value >= min(a,b) && value <= max(a,b);
}

// Within the audited single-H SAND_H domain, namespace, phase and sourceKind
// are constants. Group still compares complete sourceId/origin and tag/medium/column.
bool m2Eligible(uint row[16]) {
    return row[M2_sand16_mass] > 0u && row[M2_sand16_mass] <= M2_cfg_lightMax &&
        row[M2_sand16_tag] == 1u && row[M2_sand16_wait] == 0u && row[M2_sand16_medium] <= 1u;
}
bool m2SameGroup(uint a[16], uint b[16]) {
    return a[M2_sand16_sourceId] == b[M2_sand16_sourceId] &&
        a[M2_sand16_originHi] == b[M2_sand16_originHi] && a[M2_sand16_originLo] == b[M2_sand16_originLo] &&
        a[M2_sand16_tag] == b[M2_sand16_tag] && a[M2_sand16_medium] == b[M2_sand16_medium] &&
        (a[M2_sand16_x] >> 20u) == (b[M2_sand16_x] >> 20u);
}
bool m2Pair(uint a[16], uint b[16], uint radius) {
    uint yA = a[M2_sand16_y], yB = b[M2_sand16_y];
    return m2Eligible(a) && m2Eligible(b) && m2SameGroup(a,b) &&
        a[M2_sand16_id] < b[M2_sand16_id] && max(yA,yB) - min(yA,yB) <= radius;
}

// These snapshot operands are local. Candidate/spare, event credit, mutation
// serial and lane barriers remain the enclosing SOURCE pass's responsibility.
bool m2MergeRows(uint a[16], uint b[16], uint radius, uint sandSurface, uint eta, uint water,
                 out uint merged[16], out uint retired[16]) {
    if (!m2Pair(a,b,radius)) return false;
    for (uint f = 0u; f < 16u; ++f) {
        merged[f] = a[f];
        retired[f] = f == M2_sand16_generation ? b[f] : 0u;
    }
    merged[M2_sand16_mass] += b[M2_sand16_mass];
    for (uint f = M2_sand16_x; f <= M2_sand16_vy; ++f) {
        int v;
        if (!m2WeightedRne(a[M2_sand16_mass], int(a[f]), b[M2_sand16_mass], int(b[f]), v)) return false;
        merged[f] = uint(v);
    }
    uint y = merged[M2_sand16_y];
    int vy = int(merged[M2_sand16_vy]);
    merged[M2_sand16_medium] = water > 0u && y > sandSurface && (y < eta || (y == eta && vy <= 0)) ? 1u : 0u;
    return true;
}
