"""Exact conservative affine clock envelope from independent app-domain samples."""
from fractions import Fraction as F
from itertools import combinations


def check(ok, reason):
    if not ok:
        raise ValueError(reason)


def number(v):
    return F(str(v))


def envelope(samples, start_ns, end_ns, hold):
    check(len(samples) >= 3, 'CLOCK_MAPPING_UNKNOWN')
    scope = (hold['attemptId'], hold['holdId'], hold['clockId'], hold['bootSession'])
    rows, serial, previous_h, previous_d = [], 0, None, None
    for row in samples:
        s = row['sample']
        check(s['revision'] == 'T413_CLOCK_SAMPLE_V2' and s['unit'] == 'ms' and s['continuousForeground'] is True and
              (s['attemptId'], s['holdId'], s['clockId'], s['bootSession']) == scope, 'CLOCK_MAPPING_UNKNOWN')
        lo, hi = F(row['hostBeforeNs'], 1000000), F(row['hostAfterNs'], 1000000)
        d, error = number(s['appMonoMs']), number(row['quantizationMs'])
        check(lo <= hi and error >= 0 and type(s['sampleSerial']) is int and s['sampleSerial'] > serial and
              (previous_h is None or previous_h <= lo) and (previous_d is None or previous_d < d), 'CLOCK_MAPPING_UNKNOWN')
        check(row['source'] == 'APP_SAMPLE_HOLD_CLOCK' and row['rawCommand'] and row['clockUncertaintySource'], 'CLOCK_MAPPING_UNKNOWN')
        rows.append((lo, hi, d-error, d+error))
        serial, previous_h, previous_d = s['sampleSerial'], hi, d
    start, end = F(start_ns, 1000000), F(end_ns, 1000000)
    check(start <= end and rows[0][1] <= start and rows[-1][0] >= end, 'CLOCK_MAPPING_UNKNOWN')
    # A positive affine map needs max_i(d_i-e_i-a*h_i+) <= min_j(d_j+e_j-a*h_j-).
    amin, amax = F(0), None
    for li, hi, dl, du in rows:
        for lj, hj, el, eu in rows:
            coefficient, rhs = lj-hi, eu-dl
            if coefficient > 0:
                bound = rhs/coefficient
                amax = bound if amax is None else min(amax, bound)
            elif coefficient < 0:
                amin = max(amin, rhs/coefficient)
            else:
                check(rhs >= 0, 'CLOCK_MAPPING_UNKNOWN')
    check(amax is not None and F(0) < amin <= amax, 'CLOCK_MAPPING_UNKNOWN')
    # Breakpoints of the upper/lower envelope plus slope endpoints contain all extrema.
    lines = [(h, d) for lo, hi, dl, du in rows for h, d in ((hi, dl), (lo, du))]
    slopes = {amin, amax}
    for (h1, d1), (h2, d2) in combinations(lines, 2):
        if h1 != h2:
            a = (d1-d2)/(h1-h2)
            if amin <= a <= amax:
                slopes.add(a)
    lower, upper = [], []
    for a in slopes:
        bmin = max(dl-a*hi for lo, hi, dl, du in rows)
        bmax = min(du-a*lo for lo, hi, dl, du in rows)
        if bmin <= bmax:
            lower.append(a*start+bmin)
            upper.append(a*end+bmax)
    check(lower and upper, 'CLOCK_MAPPING_UNKNOWN')
    l, u = min(lower), max(upper)
    check(number(hold['startMonoMs']) <= l <= u <= number(hold['endMonoMs']), 'CAPTURE_INTERVAL_OUTSIDE_HOLD')
    return {'revision': 'T413_AFFINE_ENVELOPE_V2', 'lowerExactMs': str(l), 'upperExactMs': str(u),
            'rateMinExact': str(amin), 'rateMaxExact': str(amax), 'samples': len(samples)}
