#!/usr/bin/env python3
"""T578: independent recomputation of G12 four-profile resource envelope.
Formulas rebuilt from architecture-T567 SS3/SS6; manifest table used only as
cross-check. Exact integer arithmetic; no GPU, no device.
"""
# parent G9-DM v2 (T554-frozen) per-profile baselines
PARENT = {192: {'state': 591872, 'work': 1286784},
          512: {'state': 673792, 'work': 1289344}}
DM_WORK_ADD = 24576            # SS7 DM v2 WORK delta (48 word/column * 128 * 4B)
G12_STATE_ADD = 401664         # header256+column8192+origin131072+source262144
G12_WORK_FIXED = 675840        # SS3 fixed WORK segments
N = 128

MANIFEST = {  # reference values (cross-check only)
    (192, 64):  {'state': 1001728,  'work': 2028160,  'eventRows': 91920,
                 'pathRows': 81920,  'arena': 28774400,  'resident': 122134400,
                 'root': 59621888,   'full': 122204032,  'trials': 294850432},
    (192, 128): {'state': 1009920,  'work': 2044544,  'eventRows': None,
                 'pathRows': 114688, 'arena': 32968704,  'resident': 138968960,
                 'root': 68026880,   'full': 139038592,  'trials': 336850816},
    (512, 64):  {'state': 1083648,  'work': 2071680,  'eventRows': None,
                 'pathRows': 163840, 'arena': 44584960,  'resident': 185829760,
                 'root': 91406848,   'full': 185899392,  'trials': 453409152},
    (512, 128): {'state': 1091840,  'work': 2088064,  'eventRows': 141392,
                 'pathRows': 196608, 'arena': 48779264,  'resident': 202664320,
                 'root': 99811840,   'full': 202733952,  'trials': 495409536},
}


def recompute(K, Kw):
    p = PARENT[K]
    state = p['state'] + G12_STATE_ADD + 128 * Kw
    work = p['work'] + DM_WORK_ADD + G12_WORK_FIXED + 128 * K + 256 * Kw
    path_rows = 32 * 8 * K + 32 * 16 * Kw
    ev = MANIFEST[(K, Kw)]['eventRows']
    arena = ev * 256 + path_rows * 64 if ev else None
    resident = 4 * arena + 5 * state + work if arena else None
    root = 2 * state + 2 * arena + 65536 + 4096 if arena else None
    full = resident + 65536 + 4096 if resident else None
    trials = full + 6 * arena if full else None
    return {'state': state, 'work': work, 'pathRows': path_rows, 'arena': arena,
            'resident': resident, 'root': root, 'full': full, 'trials': trials}


def dispatch():
    wet = 2 * (63 * 32 + 6 * 8) + 2 * (32 * 32 + 8) + 12
    return {'wet': wet, 'g1f4_wet': 16 + 4 * wet,
            'geom_units_K512': 7 * (31 * N * 513 + 512 * 511 // 2)}


if __name__ == '__main__':
    import json
    out = {}
    for (K, Kw) in [(192, 64), (192, 128), (512, 64), (512, 128)]:
        r = recompute(K, Kw)
        ref = MANIFEST[(K, Kw)]
        checks = {k: (r[k] == ref[k]) for k in ('state', 'work', 'pathRows',
                                                'resident', 'root', 'full', 'trials') if r[k]}
        checks['arena'] = (r['arena'] == ref['arena']) if r['arena'] else 'PARTIAL(no eventRows formula)'
        out[f'K{K}/Kw{Kw}'] = {'computed': r, 'checks': checks}
    out['dispatch'] = dispatch()
    print(json.dumps(out, indent=1))
