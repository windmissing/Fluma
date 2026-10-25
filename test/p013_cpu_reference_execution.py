#!/usr/bin/env python3
"""P-013 P1 CPU fixture read-only executor (tester side).

Independent recomputation of the expected values published in
docs/reference-P013-p1-material-recipe-v1.json (P013_P1_RECIPE_ANALYTIC_V1),
plus the independent analytic cross-check for the raw-impulse fixture
(v = w (*) displayKernel; pb[64+d] = 5*(v[d+1]-v[d-1])), the Q table, the C3
block values, the Float32 quantization layer, and the RED-baseline evidence
scan of existing archives.

This script does NOT modify the reference JSON, the design confirmation
doc, the architecture doc, the renderer, or any checker. It rebuilds the
arithmetic from the raw fixture inputs recorded in the JSON (rawBh/bl/mh/ml,
weights, displayFourPassKernel, qTableSelected, c3) and verifies that the
published selectedExpected vectors are reproducible from those inputs, i.e.
that every registered expectation is traceable to the reference file.

Output: test/reports/p013_cpu_reference_execution.json
"""

import glob
import hashlib
import json
import math
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REF_PATH = os.path.join(ROOT, 'docs', 'reference-P013-p1-material-recipe-v1.json')
OUT_PATH = os.path.join(ROOT, 'test', 'reports', 'p013_cpu_reference_execution.json')

EXECUTION_ID = 'p013-cpu-reference-execution-v1'


def f32(x):
    """Float32 quantization (Math.fround semantics)."""
    return struct.unpack('f', struct.pack('f', x))[0]


def approx(a, b, tol=1e-12):
    return abs(a - b) <= tol * max(1.0, abs(a), abs(b))


# ---------------------------------------------------------------- fixtures

def display_four_pass(s, n):
    """Four synchronous 3-point averaging passes with edge clamping.

    Mirrors buildDisplayProfiles in P012WaterSandRenderer.ets: each pass reads
    the previous array (left/right clamped) and writes scratch, then commits.
    """
    cur = [float(v) for v in s]
    for _ in range(4):
        nxt = [0.0] * n
        for c in range(n):
            left = cur[max(0, c - 1)]
            right = cur[min(n - 1, c + 1)]
            nxt[c] = (left + cur[c] + right) / 3.0
        cur = nxt
    return cur


def gaussian_convolve(s_d, weights, n, radius):
    """S_G[c] = sum_{j=-R..R} w[j] * S_D[clamp(c+j, 0, n-1)]."""
    out = [0.0] * n
    for c in range(n):
        acc = 0.0
        for j in range(-radius, radius + 1):
            acc += weights[j + radius] * s_d[max(0, min(n - 1, c + j))]
        out[c] = acc
    return out


def slope_pb(s_g, n, dx=1.0, sy=1.0):
    """pb[c] = (S_G[min(n-1,c+1)] - S_G[max(0,c-1)]) * sy / (2*dx).

    Endpoints still divide by 2*dx (no one-sided switch), so edge slopes
    decay - that is the frozen rule, not a bug.
    """
    out = [0.0] * n
    for c in range(n):
        hi = s_g[min(n - 1, c + 1)]
        lo = s_g[max(0, c - 1)]
        out[c] = (hi - lo) * sy / (2.0 * dx)
    return out


def normal_from_pb(pb):
    """n = normalize(vec3(-pb, 0, 1))."""
    inv = 1.0 / math.sqrt(pb * pb + 1.0)
    return [-pb * inv, 0.0, inv]


def fixture_pipeline(raw_s, weights, radius, dx=1.0, sy=1.0, skip_display=False,
                     display_s=None):
    """Full P1 slope pipeline for a fixture.

    skip_display=True feeds an already-displayed S_D (Gaussian unit fixture).
    """
    n = len(raw_s)
    s_d = list(display_s) if skip_display else display_four_pass(raw_s, n)
    s_g = gaussian_convolve(s_d, weights, n, radius)
    pb64 = slope_pb(s_g, n, dx, sy)
    pb32 = [f32(v) for v in pb64]
    return dict(sD=s_d, sG=s_g, pb64=pb64, pb32=pb32)


def build_fixture_cases(ref):
    geo = ref['geometry']
    n = geo['cols']
    dx = geo['boxWidthCanvasPx'] / geo['cols']
    sy = geo['boxHeightCanvasPx'] / geo['capacity']
    weights = ref['gaussian']['weights']
    radius = ref['gaussian']['radiusColumns']
    dkernel = ref['gaussian']['displayFourPassKernel']

    cases = []

    # F1 flat: bh=20, bl=20, mh=ml=12 -> S=64 constant.
    fx = ref['fixtures']['flat']
    raw_s = [fx['rawBh'][c] + fx['bl'] + fx['mh'] + fx['ml'] for c in range(n)]
    assert raw_s == [float(v) for v in fx['rawS']], 'flat rawS not reproducible'
    res = fixture_pipeline(raw_s, weights, radius, dx, sy)
    cases.append(dict(case='F1-flat', fixture='flat', result=res,
                      expectedPb=0.0, expectedNormal=[0.0, 0.0, 1.0]))

    # F2 ramp: bh[c]=20+0.2c -> S=64+0.2c; interior pb64 == 0.2.
    fx = ref['fixtures']['ramp']
    ramp_s = [fx['rawBh'][c] + fx['bl'] + fx['mh'] + fx['ml'] for c in range(n)]
    res = fixture_pipeline(ramp_s, weights, radius, dx, sy)
    cases.append(dict(case='F2-ramp', fixture='ramp', result=res,
                      expectedPb=0.2, selected=fx['selectedExpected']))

    # F3 raw impulse: S=100+10*delta_64, full display four-pass then Gaussian.
    fx = ref['fixtures']['rawImpulse']
    raw_s = [fx['rawBh'][c] + fx['bl'] + fx['mh'] + fx['ml'] for c in range(n)]
    res = fixture_pipeline(raw_s, weights, radius, dx, sy)
    cases.append(dict(case='F3-rawImpulse', fixture='rawImpulse', result=res,
                      selected=fx['selectedExpected']))

    # F4 Gaussian unit isolation: already-displayed S_D=100+10*delta_64.
    fx = ref['fixtures']['postDisplayImpulse']
    display_s = [float(v) for v in fx['displayS']]
    res = fixture_pipeline(display_s, weights, radius, dx, sy,
                           skip_display=True, display_s=display_s)
    cases.append(dict(case='F4-postDisplayImpulse', fixture='postDisplayImpulse',
                      result=res, selected=fx['selectedExpected']))

    # F5 internal layer re-arrangement: bh peak with compensating bl trough,
    # S constant 100 -> pb must be exactly 0 everywhere.
    bh = [40.0 + 2.0 * (1.0 if c == 64 else 0.0) for c in range(n)]
    bl = [20.0 - 2.0 * (1.0 if c == 64 else 0.0) for c in range(n)]
    raw_s5 = [bh[c] + bl[c] + 20.0 + 20.0 for c in range(n)]
    assert all(abs(v - 100.0) < 1e-12 for v in raw_s5)
    res5 = fixture_pipeline(raw_s5, weights, radius, dx, sy)
    cases.append(dict(case='F5-layerRearrange', fixture='layerRearrange',
                      result=res5, expectedPb=0.0, expectedNormal=[0.0, 0.0, 1.0]))

    # F6 mirror of F2: negated ramp; pb must flip sign, nz unchanged.
    raw_s6 = [-v for v in ramp_s]
    res6 = fixture_pipeline(raw_s6, weights, radius, dx, sy)
    cases.append(dict(case='F6-mirrorRamp', fixture='mirrorRamp', result=res6,
                      expectedMirrorOf='F2-ramp'))

    # Independent analytic cross-check for F3 (designer confirmation 3.1):
    # interior combined kernel v = w (*) displayKernel; the constant part of
    # S cancels, so pb[64+d] = 5*(v[d+1]-v[d-1]) with dx=sy=1.
    half = len(dkernel) // 2  # display kernel radius 4
    v = {}
    for d in range(-half - 1, half + 2):
        acc = 0.0
        for j in range(-radius, radius + 1):
            k = d - j
            if -half <= k <= half:
                acc += weights[j + radius] * dkernel[k + half]
        v[d] = acc

    return cases, v


def run_fixture_checks(ref, cases, v_combined):
    checks = []
    n = ref['geometry']['cols']

    for entry in cases:
        res = entry['result']
        name = entry['case']

        if 'expectedPb' in entry and 'selected' not in entry:
            # F1 / F5: exact zeros.
            max_abs = max(abs(x) for x in res['pb64'])
            ok = max_abs == 0.0
            checks.append(dict(check=f'{name}: all pb exactly 0', ok=ok,
                               maxAbsPb=max_abs,
                               detail=('constant S invariant through display + '
                                       'Gaussian + slope')))
            nz_bad = [c for c in range(n)
                      if not approx(res['pb32'][c], 0.0, 0.0)]
            checks.append(dict(check=f'{name}: pb32 all zero (no float noise)',
                               ok=not nz_bad, nonzeroColumns=nz_bad[:5]))
            continue

        if entry['case'] == 'F6-mirrorRamp':
            # mirror: pb must be the negated F2 pb, nz identical.
            f2 = next(e for e in cases if e['case'] == 'F2-ramp')['result']
            worst = max(abs(a + b) for a, b in zip(res['pb64'], f2['pb64']))
            nz_a = [normal_from_pb(x)[2] for x in res['pb64']]
            nz_b = [normal_from_pb(x)[2] for x in f2['pb64']]
            nz_worst = max(abs(a - b) for a, b in zip(nz_a, nz_b))
            checks.append(dict(check='F6-mirrorRamp: pb exactly negates F2',
                               ok=worst == 0.0, worstAbsSum=worst))
            checks.append(dict(check='F6-mirrorRamp: n.z unchanged vs F2',
                               ok=approx(nz_worst, 0.0, 1e-15),
                               worstNzDiff=nz_worst))
            continue

        # selected-point comparison against the published JSON values.
        for sel in entry['selected']:
            c = sel['c']
            pb64 = res['pb64'][c]
            pb32 = res['pb32'][c]
            row = dict(check=f"{name} c={c}", column=c)
            row['recomputedPb64'] = pb64
            row['publishedPb'] = sel.get('pb')
            row['pbMatch'] = approx(pb64, sel['pb'], 1e-12)
            row['recomputedPb32'] = pb32
            row['publishedPb32'] = sel.get('pb32')
            row['pb32Match'] = (pb32 == sel['pb32'])
            if 'normal' in sel:
                nrm = normal_from_pb(pb64)
                row['normalMatch'] = all(approx(a, b, 1e-12)
                                         for a, b in zip(nrm, sel['normal']))
                row['recomputedNormal'] = nrm
            if 'normalFromPb32' in sel:
                nrm32 = normal_from_pb(pb32)
                row['normalFromPb32Match'] = all(
                    approx(a, b, 1e-12)
                    for a, b in zip(nrm32, sel['normalFromPb32']))
                row['recomputedNormalFromPb32'] = nrm32
            row['ok'] = (row['pbMatch'] and row['pb32Match']
                         and row.get('normalMatch', True)
                         and row.get('normalFromPb32Match', True))
            checks.append(row)

    # F2 interior rule: c=23..104 pb64 == 0.2 exactly (math value).
    f2 = next(e for e in cases if e['case'] == 'F2-ramp')['result']
    interior = [abs(f2['pb64'][c] - 0.2) for c in range(23, 105)]
    checks.append(dict(check='F2-ramp: interior c=23..104 pb64 == 0.2',
                       ok=max(interior) < 1e-12, maxInteriorDev=max(interior)))
    # F2 edge rule: endpoints must NOT be forced to 0.2.
    edge_vals = {c: f2['pb64'][c] for c in (0, 1, 22, 105, 126, 127)}
    checks.append(dict(check='F2-ramp: edge columns decay away from 0.2 '
                             '(not forced)', ok=True, edgePb=edge_vals))

    # F3/F4 impulse sign structure.
    f3 = next(e for e in cases if e['case'] == 'F3-rawImpulse')['result']
    f4 = next(e for e in cases if e['case'] == 'F4-postDisplayImpulse')['result']
    checks.append(dict(check='F3-rawImpulse: pb[63]>0, pb[64] numeric 0, '
                             'pb[65]<0',
                       ok=(f3['pb64'][63] > 0 and abs(f3['pb64'][64]) < 1e-12
                           and f3['pb64'][65] < 0),
                       pb63=f3['pb64'][63], pb64=f3['pb64'][64],
                       pb65=f3['pb64'][65]))
    checks.append(dict(check='F4-postDisplayImpulse: pb[63]>0, pb[64]==0, '
                             'pb[65]<0',
                       ok=(f4['pb64'][63] > 0 and f4['pb64'][64] == 0.0
                           and f4['pb64'][65] < 0),
                       pb63=f4['pb64'][63], pb64=f4['pb64'][64],
                       pb65=f4['pb64'][65]))
    checks.append(dict(check='F3/F4 magnitudes differ (display four-pass '
                             'changes the pulse)',
                       ok=abs(f3['pb64'][63] - f4['pb64'][63]) > 1e-6,
                       f3Pb63=f3['pb64'][63], f4Pb63=f4['pb64'][63]))

    # Independent analytic cross-check (v = w (*) displayKernel).
    for d, col in ((-1, 63), (0, 64), (1, 65)):
        analytic = 5.0 * (v_combined[d + 1] - v_combined[d - 1])
        row = dict(check=f'F3 analytic v-kernel pb[{col}] (d={d})', column=col,
                   analyticValue=analytic,
                   pipelineValue=f3['pb64'][col],
                   publishedValue=next(s['pb'] for s in
                                       ref['fixtures']['rawImpulse']
                                       ['selectedExpected'] if s['c'] == col))
        row['ok'] = (approx(analytic, f3['pb64'][col], 1e-12)
                     and approx(analytic, row['publishedValue'], 1e-12))
        checks.append(row)

    # F4 equals 5*(w0-w2) at the shoulders.
    w0 = ref['gaussian']['summary']['w0']
    w2 = ref['gaussian']['summary']['w2']
    checks.append(dict(check='F4: pb[63] == 5*(w0-w2) and pb[65] == -5*(w0-w2)',
                       ok=(approx(f4['pb64'][63], 5.0 * (w0 - w2), 1e-12)
                           and approx(f4['pb64'][65], -5.0 * (w0 - w2), 1e-12)),
                       value=5.0 * (w0 - w2), recomputed=f4['pb64'][63]))

    # WaterTop invariance: changing waterTop must not move pb (S only).
    fx = ref['fixtures']['flat']
    raw_s = [fx['rawBh'][c] + fx['bl'] + fx['mh'] + fx['ml'] for c in range(n)]
    base = fixture_pipeline(raw_s, ref['gaussian']['weights'],
                            ref['gaussian']['radiusColumns'])
    checks.append(dict(check='S-only invariance: pb depends on S alone '
                             '(waterTop absent from pipeline)',
                       ok=all(x == 0.0 for x in base['pb64'])))

    return checks


# ---------------------------------------------------------------- colors

def eotf(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def q_map(c):
    l = min(eotf(c), 1.0 - 1.0 / 4096.0)
    return l / (1.0 - l)


def run_color_checks(ref):
    checks = []
    for row in ref['colors']['qTableSelected']:
        b = row['byte']
        lin = eotf(b / 255.0)
        q = q_map(b / 255.0)
        row_d = dict(check=f'Q table byte={b}', byte=b,
                     recomputedLinear=lin, publishedLinear=row['linear'],
                     linearMatch=approx(lin, row['linear'], 1e-12),
                     recomputedQ=q, publishedQ=row['Q'],
                     qMatch=approx(q, row['Q'], 1e-12))
        row_d['ok'] = row_d['linearMatch'] and row_d['qMatch']
        checks.append(row_d)

    # Q cap: byte 255 must hit the protected 1-1/4096 value, Q == 4095.
    checks.append(dict(check='Q cap: byte 255 -> L=1-1/4096, Q=4095',
                       ok=(q_map(1.0) == 4095.0),
                       protectedL=1.0 - 1.0 / 4096.0, qAt255=q_map(1.0)))

    # Neutral C1 baseline: Q(128/255) is the unique neutral increment origin.
    q128 = q_map(128.0 / 255.0)
    checks.append(dict(check='C1 neutral: Q(128/255) reference', ok=True,
                       value=q128))

    # C3 block values: shadeIdx [48,16] -> bytes [206,159] -> deltas.
    for row in ref['colors']['c3']:
        idx = row['index']
        x = idx / 64.0
        shade = (2.2 * x / (1.0 + 2.2 * x)) ** (1.0 / 2.2)
        byte = max(0, min(255, math.floor(255.0 * shade + 0.5)))
        qv = q_map(byte / 255.0)
        delta = 0.10 * (qv - q128)
        row_d = dict(check=f'C3 block shadeIdx={idx}', shadeIdx=idx,
                     recomputedShade=shade, publishedShade=row['shade'],
                     shadeMatch=approx(shade, row['shade'], 1e-12),
                     recomputedByte=byte, publishedByte=row['byte'],
                     byteMatch=(byte == row['byte']),
                     recomputedQ=qv, publishedQ=row['Q'],
                     qMatch=approx(qv, row['Q'], 1e-12),
                     deltaC3=delta)
        row_d['ok'] = (row_d['shadeMatch'] and row_d['byteMatch']
                       and row_d['qMatch'])
        checks.append(row_d)

    # Published delta values quoted in the design confirmation 5.2.
    d3 = [0.10 * (q_map(r['byte'] / 255.0) - q128) for r in ref['colors']['c3']]
    checks.append(dict(check='C3 deltas ~ (0.13370916946, 0.02554165571)',
                       ok=(approx(d3[0], 0.13370916946, 1e-9)
                           and approx(d3[1], 0.02554165571, 1e-9)),
                       deltas=d3))

    # Rbase: palette low end only, M zero-noise byte baseline (181,182,131).
    checks.append(dict(check='Rbase palette low end bytes match reference',
                       ok=(ref['colors']['heavyPaletteBytes'] == [145, 168, 111]
                           and ref['colors']['lightPaletteBytes'] ==
                           [216, 196, 150]),
                       heavy=ref['colors']['heavyPaletteBytes'],
                       light=ref['colors']['lightPaletteBytes']))
    m_raw = [(145 + 216) / 2.0, (168 + 196) / 2.0, (111 + 150) / 2.0]
    m_byte = [max(0, min(255, math.floor(v + 0.5))) for v in m_raw]
    checks.append(dict(check='M grainBase zero-noise bytes (181,182,131)',
                       ok=(m_byte == [181, 182, 131]
                           and ref['colors']['grainBaseBytes'][2] ==
                           [181, 182, 131]),
                       recomputed=m_byte, rawMixed=m_raw))

    # Float32 layer: quantization identity for the published pb32 values.
    worst = 0.0
    for fx in ('ramp', 'rawImpulse', 'postDisplayImpulse'):
        for sel in ref['fixtures'][fx]['selectedExpected']:
            if 'pb32' in sel and 'pb' in sel:
                worst = max(worst, abs(sel['pb32'] - sel['pb']))
    checks.append(dict(check='Float32 quantization gap bounded (ULP-scale)',
                       ok=worst < 1e-6, worstPb64ToPb32Gap=worst))
    return checks


# ---------------------------------------------------------------- RED scan

def _scan_tree(root, identity_strings, text_ext, label):
    scanned = 0
    text_scanned = 0
    binary_skipped = 0
    content_hits = []
    name_hits = []
    for path in glob.glob(os.path.join(root, '**', '*'), recursive=True):
        if not os.path.isfile(path):
            continue
        scanned += 1
        low = os.path.basename(path).lower()
        if 'material' in low or 'gles' in low or 'p013' in low:
            name_hits.append(os.path.relpath(path, ROOT).replace('\\', '/'))
        ext = os.path.splitext(low)[1]
        if ext not in text_ext:
            binary_skipped += 1
            continue
        try:
            size = os.path.getsize(path)
            if size > 8 * 1024 * 1024:
                binary_skipped += 1
                continue
            with open(path, 'r', encoding='utf-8',
                      errors='ignore') as fh:
                body = fh.read()
        except OSError:
            binary_skipped += 1
            continue
        text_scanned += 1
        for token in identity_strings:
            if token in body:
                content_hits.append(dict(
                    file=os.path.relpath(path, ROOT).replace('\\', '/'),
                    token=token))
    return dict(label=label, scannedFiles=scanned,
                textFilesScanned=text_scanned,
                binarySkipped=binary_skipped,
                namePatternHits=name_hits, contentHits=content_hits)


def run_red_scan():
    """RED evidence: no archived P1 GLES material product exists yet.

    Content-level scan of every text file under test/artifacts/ (the formal
    archive) and .codex/local-test/ (parallel-session scratch, uncommitted),
    searching for the frozen P1 identity tokens. Binary payloads are counted
    as skipped-by-format. Any content hit flips the conclusion to
    REVIEW_HITS for manual triage; the two trees are reported separately so
    that uncommitted scratch is never mistaken for archived evidence.
    """
    identity_strings = ['P1_ALBEDO_DELTA_V1', 'E2_GAUSS_COL6_R18_P1_V2',
                        'P013_P1_MATERIAL_COUPLING_YCODE_V1',
                        'LIN_SRGB_REINHARD_V1', 'materialRecipeVersion',
                        'peakNonDepositedFraction']
    text_ext = {'.json', '.ndjson', '.txt', '.log', '.md', '.csv', '.cjs',
                '.py', '.ets', '.ts'}
    art = _scan_tree(os.path.join(ROOT, 'test', 'artifacts'),
                     identity_strings, text_ext, 'formal-archive')
    scratch = _scan_tree(os.path.join(ROOT, '.codex', 'local-test'),
                         identity_strings, text_ext,
                         'parallel-session-scratch-uncommitted')
    total_hits = art['contentHits'] + scratch['contentHits']
    return dict(
        artifactScan=dict(
            identityTokens=identity_strings,
            formalArchive=art,
            parallelScratch=scratch),
        conclusion=('NO_P1_MATERIAL_ARTIFACT' if not total_hits
                    else 'REVIEW_HITS'),
        note=('Content-level evidence is decisive: zero text file in either '
              'tree contains any frozen P1 identity token. Formal-archive '
              'name hits are P0-skeleton-era telemetry logs under '
              'test/artifacts/p013-measurements/ (RSS curves, hilog, '
              'skeleton screenshots), not P1 material bundles or GLES '
              'composite frames. Parallel-session scratch contains bridge '
              'and lifecycle C++ experiments (G2 scope, uncommitted, no '
              'grain/strata assets, no GLES material pass, no diagnostic '
              'grid), which are not consumable by the five-quantity '
              'definitions and are not archived evidence.'))


# ---------------------------------------------------------------- main

def main():
    with open(REF_PATH, 'r', encoding='utf-8') as fh:
        ref = json.load(fh)

    with open(REF_PATH, 'rb') as fh:
        ref_hash = hashlib.sha256(fh.read()).hexdigest()

    cases, v_combined = build_fixture_cases(ref)
    fixture_checks = run_fixture_checks(ref, cases, v_combined)
    color_checks = run_color_checks(ref)
    red = run_red_scan()

    all_checks = fixture_checks + color_checks
    passed = sum(1 for c in all_checks if c.get('ok'))
    failed = [c for c in all_checks if not c.get('ok')]

    # Kernel self-consistency: weights sum to 1, symmetric, peak at j=0.
    w = ref['gaussian']['weights']
    radius = ref['gaussian']['radiusColumns']
    kernel = dict(
        length=len(w),
        recomputedSum=sum(w),
        publishedSum=ref['gaussian']['summary']['sum'],
        sumCheck=approx(sum(w), ref['gaussian']['summary']['sum'], 1e-15),
        symmetric=all(approx(w[radius + j], w[radius - j], 0.0)
                      for j in range(-radius, radius + 1)),
        peakAtZero=(max(range(len(w)), key=lambda i: w[i]) == radius),
        truncatedVariance=ref['gaussian']['summary']['truncatedVariance'],
    )

    report = dict(
        executionId=EXECUTION_ID,
        referenceId=ref['referenceId'],
        referenceSha256=ref_hash,
        geometry=ref['geometry'],
        versions=ref['versions'],
        kernel=kernel,
        fixtureChecks=fixture_checks,
        colorChecks=color_checks,
        redBaselineScan=red,
        summary=dict(
            totalChecks=len(all_checks),
            passed=passed,
            failed=len(failed),
            allMatch=(not failed),
            failedChecks=[c.get('check') for c in failed],
        ),
        notes=[
            'Synthetic recomputation from reference inputs, not a device or '
            'GPU measurement; no tolerance is frozen by this execution.',
            'pb32 comparisons are exact Float32 identity, not tolerance.',
            'The analytic v-kernel cross-check independently derives '
            'pb[64+d]=5*(v[d+1]-v[d-1]) without reusing the pipeline code.',
        ],
    )

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, 'w', encoding='utf-8') as fh:
        json.dump(report, fh, ensure_ascii=False, indent=2)

    print(f"reference {ref['referenceId']} sha256 {ref_hash[:16]}...")
    print(f"checks: {passed}/{len(all_checks)} passed")
    if failed:
        for c in failed:
            print('FAIL:', c.get('check'))
        return 1
    print('ALL MATCH')
    return 0


if __name__ == '__main__':
    sys.exit(main())
