"""T356 designer arithmetic/specification model; never imports/runs product code.

Synthetic scalar tile and synthetic published fixtures, not production assets,
GPU execution, physical trajectories, device tests or a visual-quality PASS.
Run from any directory with Python + numpy + Pillow.
"""
from pathlib import Path
import hashlib
import json
import math

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
HERE = ROOT / 'docs'
A8 = np.array([145, 168, 111], dtype=np.float64)
BG8 = np.array([16, 16, 36], dtype=np.float64)
YWEIGHT = np.array([.2126, .7152, .0722])
Q = np.arange(256, dtype=np.float64) / 255
N, CAP = 128, 256


def eotf(c):
    c = np.asarray(c, dtype=np.float64)
    return np.where(c <= .04045, c/12.92, ((c+.055)/1.055)**2.4)


def oetf(c):
    c = np.asarray(c, dtype=np.float64)
    assert np.isfinite(c).all() and (c >= 0).all() and (c <= 1).all()
    return np.where(c <= .0031308, c*12.92, 1.055*c**(1/2.4)-.055)


def byte(c):
    return np.floor(oetf(c)*255+.5).astype(np.uint8)


A, BG = eotf(A8/255), eotf(BG8/255)
LIGHT = np.array([-.35, 0, .82]); LIGHT /= np.linalg.norm(LIGHT)


def particle_gain(rho, core=.10, rim=.16):
    t = np.clip((rho-.60)/.35, 0, 1)
    return 1+core*(1-2*rho*rho)-rim*t*t*(3-2*t)


def synthetic_tile():
    """Balanced synthetic byte fixture, deliberately NOT the production generator."""
    y, x = np.indices((256, 256), dtype=np.uint32)
    h = (x*np.uint32(0x27d4eb2d)) ^ (y*np.uint32(0x165667b1))
    h = h*np.uint32(0x9e3779b1)
    k = ((h >> np.uint32(16)) & np.uint32(255)).astype(np.float64)
    # Smooth correlated grain at about a few logical pixels, bounded by +/-1.
    u = (k-127.5)/127.5
    u = sum(np.roll(np.roll(u, iy, 0), ix, 1)
            for iy in [-1, 0, 1] for ix in [-1, 0, 1])/9
    u = np.clip(u*2, -1, 1)
    encoded = np.floor(128 + np.where(u < 0, 128, 127)*u+.5).astype(np.uint8)
    return encoded


TILE = synthetic_tile()


def normal_profile(display, width, height):
    j = np.arange(-18, 19, dtype=np.float64)
    w = np.exp(-j*j/72); w /= w.sum()
    padded = np.pad(display, 18, mode='edge')
    smoothed = np.convolve(padded, w, mode='valid')
    # Endpoint denominator remains 2*dx, matching the frozen operator.
    pb = (smoothed[np.minimum(np.arange(N)+1, N-1)] -
          smoothed[np.maximum(np.arange(N)-1, 0)])*(height/CAP)/(2*width/N)
    return pb.astype(np.float32).astype(np.float64)


def render(profile, particles, scale=1, grain=.08, face=.10,
           core=.10, rim=.16, wrong_hue=False, halo=False, sparkle=False,
           frame=0, shift=(0, 0)):
    """profile is an already-displayed S_D fixture, so no second DISPLAY filter."""
    width = height = int(256*scale)
    yy, xx = np.indices((height, width), dtype=np.float64)
    xx += .5; yy += .5
    lx, ly = xx/scale, yy/scale
    h = (1-yy/height)*CAP
    col = np.clip(xx/width*N, 0, N-1)
    sand = np.interp(col, np.arange(N), profile)
    bed = h < sand
    pb = np.interp(col, np.arange(N), normal_profile(profile, width, height))
    ndotl = (-pb*LIGHT[0]+LIGHT[2])/np.sqrt(1+pb*pb)
    delta = np.maximum(0, ndotl)-LIGHT[2]
    k = TILE[np.floor(ly).astype(int) % 256,
             np.floor(lx).astype(int) % 256].astype(float)
    noise = np.where(k <= 128, (k-128)/128, (k-128)/127)
    g = 1+grain*noise+face*np.exp(-np.maximum(0, sand-h)*height/CAP/(8*scale))*delta
    color = np.broadcast_to(BG, (height, width, 3)).copy()
    color[bed] = (A*g[..., None])[bed]
    before = color.copy()
    masks, covered, radial = [], np.zeros((height, width), dtype=bool), []
    for pid, px, py, state in particles:
        if state == 0:
            continue
        cx, cy = px/N*width+shift[0]*scale, (1-py/CAP)*height+shift[1]*scale
        r = 2*scale
        rho = np.hypot(xx-cx, yy-cy)/r
        mask = rho <= 1
        if state == 1 and py < np.interp(np.clip(px, 0, N-1), np.arange(N), profile):
            mask[:] = False
        if halo:
            ring = (rho > 1) & (rho <= 1.6)
            color[ring] = eotf(np.array([230, 240, 245])/255)
        gp = particle_gain(np.minimum(rho, 1), core, rim)
        base = eotf(np.array([205, 134, 67])/255) if wrong_hue else A
        color[mask] = (base*gp[..., None])[mask]
        masks.append((pid, mask)); radial.append(rho)
        covered |= mask
    if sparkle:
        # Deliberate negative: off-white, frame-dependent points on static bed.
        dots = bed & (((xx.astype(int)*17+yy.astype(int)*31+frame*13) % 257) == 0)
        color[dots] = eotf(np.array([248, 248, 248])/255)
    assert np.isfinite(color).all() and (color >= 0).all() and (color <= 1).all()
    rgb, baseline = byte(color), byte(before)
    return {'rgb': rgb, 'base': baseline, 'bed': bed, 'particles': masks,
            'coverage': bed | covered, 'gbed': g, 'linear': color}


def ycode(rgb):
    return rgb.astype(np.float64) @ YWEIGHT


def overlap_stats(im):
    d = np.abs(ycode(im['rgb'])-ycode(im['base']))
    return [{'id': pid, 'support_native_pixels': int(m.sum()),
             'fraction_delta_ge_4': float(np.mean(d[m] >= 4)) if m.any() else None,
             'median_delta_code': float(np.median(d[m])) if m.any() else None}
            for pid, m in im['particles']]


def main():
    checks = {}
    assert np.array_equal(np.floor(oetf(eotf(Q))*255+.5).astype(int), np.arange(256))
    checks['all_256_srgb_neutral_roundtrips'] = 256
    rho = np.linspace(0, 1, 10001)
    pg = particle_gain(rho)
    assert abs(pg.min()-.74) < 1e-12 and abs(pg.max()-1.1) < 1e-12
    checks['particle_gain_samples'] = len(rho)
    for g in np.linspace(.74, 1.18, 20001):
        c = A*g
        assert np.max(np.abs(c/c.sum()-A/A.sum())) < 5e-16
        assert (c >= 0).all() and (c <= 1).all()
    checks['linear_chromaticity_gamut_samples'] = 20001
    min_y, bg_y = float((A*.74)@YWEIGHT), float(BG@YWEIGHT)
    contrast = (min_y+.05)/(bg_y+.05)
    assert contrast >= 4
    checks['default_contrast_candidate_ge_4'] = True
    flat, empty = np.full(N, 70.), np.zeros(N)
    curve = 62+10*np.sin(np.arange(N)*2*math.pi/N)
    # IDs/states are fixture inputs, not dynamically produced particles.
    airborne = [(i+1, 12+i*7.5, 230-(i%5)*24, 1) for i in range(14)]
    contact = [(101+i, 14+i*12, 68.8, 2) for i in range(9)]
    mixed = render(flat, contact+airborne)
    neutral = render(flat, contact+airborne, grain=0, face=0, core=0, rim=0)
    plain = render(flat, contact, grain=0, face=0, core=0, rim=0)
    detailed_contact = render(flat, contact, grain=0, face=0)
    # A fully embedded state2 fixture isolates material discrimination from background.
    # The near-surface contacts in the overview are a separate, partly exposed fixture.
    embedded = [(pid, px, 66., state) for pid, px, py, state in contact]
    embedded_plain = render(flat, embedded, grain=0, face=0, core=0, rim=0)
    embedded_detail = render(flat, embedded, grain=0, face=0)
    neutral_stats, detail_stats = overlap_stats(embedded_plain), overlap_stats(embedded_detail)
    assert all(v['median_delta_code'] == 0 and v['fraction_delta_ge_4'] == 0
               for v in neutral_stats)
    assert all(v['support_native_pixels'] >= 8 and v['fraction_delta_ge_4'] >= .4
               for v in detail_stats)
    checks['fully_overlapping_contact_positive_and_flat_negative'] = len(contact)
    assert all(np.array_equal(im['coverage'], mixed['coverage']) for im in
               [neutral, render(flat, contact+airborne, grain=0, face=0),
                render(flat, contact+airborne, core=0, rim=0)])
    checks['ablation_coverage_invariance'] = 3
    for s in [1, 2, 3]:
        for dx, dy in [(0, 0), (.25, 0), (0, .25), (.25, .25)]:
            r = render(empty, [(1, 64, 180, 1)], s, shift=(dx, dy))
            mask = r['particles'][0][1]
            assert mask.any()
            # Occupied bounds lie inside the analytically supported radius; no halo.
            yy, xx = np.where(mask)
            cx, cy, radius = (128+dx)*s, (76+dy)*s, 2*s
            assert (np.hypot(xx+.5-cx, yy+.5-cy) <= radius+1e-12).all()
    checks['native_scale_subpixel_phase_support'] = 12
    static = render(curve, airborne)
    assert np.array_equal(static['rgb'], render(curve, airborne, frame=99)['rgb'])
    sparks0 = render(curve, airborne, sparkle=True)
    sparks1 = render(curve, airborne, sparkle=True, frame=1)
    assert not np.array_equal(sparks0['rgb'], sparks1['rgb'])
    halo = render(flat, contact+airborne, halo=True)
    assert np.any(np.any(halo['rgb'] != mixed['rgb'], axis=2) & ~mixed['coverage'])
    wrong = render(flat, contact+airborne, wrong_hue=True)
    assert not np.array_equal(wrong['rgb'], mixed['rgb'])
    checks['negative_halo_wrong_hue_temporal_spark_detected'] = 3
    a = render(flat, [])
    b = render(flat, [], grain=0, face=0)
    roi = np.zeros_like(a['bed']); roi[-55:-5, 8:-8] = True
    diff = ycode(a['rgb'])[roi]-ycode(b['rgb'])[roi]
    rms = float(np.sqrt(np.mean(diff**2)))
    assert .5 <= rms <= 8
    checks['synthetic_grain_candidate_rms_in_0p5_to_8'] = True
    # Neutral helper verifies legacy band-mix and double-gamma counterexamples.
    old_middle = (A8 + np.array([216, 196, 150]))*.5
    assert not np.array_equal(np.floor(old_middle+.5), A8)
    assert not np.array_equal(byte(eotf(A)), A8.astype(np.uint8))
    checks['legacy_mix_and_double_eotf_counterexamples'] = 2
    assert 5*65536+6*512+2048+37*8 == 333096
    checks['incremental_payload_template_bytes'] = 333096
    thin = render(np.full(N, .15), [])
    assert not thin['bed'].any()  # no pixel center below y=.15 at s=1
    checks['subpixel_bed_not_artificially_thickened'] = True
    scenes = [
        ('V01 EMPTY', render(empty, [])),
        ('V02 THICK BED', render(curve, [])),
        ('V03 AIR PARTICLES', render(empty, airborne)),
        ('V04 CONTACT / SAME MATERIAL', mixed),
        ('V05 THIN BED 0.15u', thin),
        ('V06 NEG: NO DETAIL', neutral),
        ('V07 NEG: WRONG HUE', wrong),
        ('V08 NEG: EXTERNAL HALO', halo),
        ('V09 NEG: WHITE SPARKS', sparks0),
    ]
    board = Image.new('RGB', (3*264+8, 3*294+55+160), '#e8e8e8')
    draw = ImageDraw.Draw(board)
    draw.text((10, 8), 'T356 MATHEMATICAL SPECIFICATION - NOT PRODUCT / DEVICE OUTPUT', fill='black')
    draw.text((10, 25), 'Synthetic scalar tile. Geometry and per-ID positions are fixed fixture inputs.', fill='black')
    for i, (label, im) in enumerate(scenes):
        x, y = 8+(i % 3)*264, 55+(i//3)*294
        draw.text((x, y), label, fill='black')
        board.paste(Image.fromarray(im['rgb']), (x, y+20))
    zoom_y = 3*294+60
    draw.text((10, zoom_y), 'FULL BED OVERLAP - 4x nearest-neighbor explanation only; NOT measurement scale', fill='black')
    draw.text((10, zoom_y+18), 'DETAIL: same albedo / inner rim', fill='black')
    draw.text((400, zoom_y+18), 'NEGATIVE: same albedo / no detail', fill='black')
    for x, im in [(10, embedded_detail), (400, embedded_plain)]:
        crop = Image.fromarray(im['rgb']).crop((20, 178, 52, 202))
        board.paste(crop.resize((128, 96), Image.Resampling.NEAREST), (x, zoom_y+36))
    image_path = HERE/'design-T356-single-sand-reference-v1.png'
    board.save(image_path)
    sources = [
        'docs/test-report-T347-v2-settle-device-v1.md',
        'docs/test-report-T350-touch-entry-device-v1.md',
        'test/reports/t347/s1.jpeg', 'test/reports/t350/B2_particle1_ext.jpeg',
        'test/reports/t350/A_default.jpeg',
        'docs/design-P013-p1-material-recipe-slope-confirmation-v1.md',
        'docs/architecture-P013-dualfield-particle-shader-adapter-v1.md',
        'docs/architecture-P013-new-pipeline-only-revision-v1.md',
        'docs/algorithm-P013-T99-basebyte-representation-v1.md',
        'docs/architecture-P013-T105-base-strip-and-color-routing-v1.md',
        'docs/architecture-T349-v2-frame-budget-v1.md',
        'entry/src/main/cpp/p013renderer/dualfield_pass.cpp',
        'entry/src/main/cpp/p013renderer/scene_gpu.cpp',
        'entry/src/main/cpp/p013renderer/scene_layers.cpp',
        'entry/src/main/cpp/p013renderer/glsl/p1_material.frag',
        'entry/src/main/ets/render/P013SceneController.ets',
        'entry/src/main/ets/render/P013ScenePreparation.ets',
        'entry/src/main/ets/render/preparation/P013SceneNumericAssets.ets',
        'entry/src/main/ets/physics/derived/P051DerivedDemoV2.ets',
        'docs/reference-P011-45deg-target-effect-v1.md',
        'docs/algorithm-T356-single-sand-dual-material-v1.md',
        'docs/retro-T356-single-sand-dual-material-v1.md',
        'docs/design-T356-single-sand-model-v1.py',
    ]
    evidence = {
        'date': '2026-11-05', 'role': 'designer',
        'route': 'single-sand-dual-material-v1',
        'status': 'DESIGN_ARITHMETIC_CHECKED_NOT_PRODUCT_VALIDATED',
        'execution': 'Python_numpy_Pillow_MATH_ONLY',
        'no_device': True, 'no_product_or_shader_execution': True,
        'tile': {'kind': 'SYNTHETIC_FIXTURE_NOT_PRODUCTION_R8',
                 'sha256': hashlib.sha256(TILE.tobytes()).hexdigest()},
        'checks': checks,
        'measurements': {'default_min_linear_contrast_conservative_g0p74': contrast,
                         'synthetic_grain_delta_Ycode_rms': rms,
                         'fully_overlapping_contact': detail_stats,
                         'same_contact_no_detail_negative': neutral_stats,
                         'contact_measurement_fixture': {'profile_u': 70, 'centers_y_u': 66,
                                                         'radius_logical_px': 2, 'scale': 1,
                                                         'all_discs_inside_bed': True},
                         'overview_contact_fixture': {'profile_u': 70, 'centers_y_u': 68.8,
                                                      'note': 'partly exposed, separate from measurement fixture'},
                         'particle_gain_min_max': [float(pg.min()), float(pg.max())],
                         'neutral_albedo': A8.tolist()},
        'figure': {'path': image_path.relative_to(ROOT).as_posix(),
                   'sha256': hashlib.sha256(image_path.read_bytes()).hexdigest(),
                   'kind': 'MATHEMATICAL_SPECIFICATION_NOT_DEVICE_SCREENSHOT',
                   'scenes': [label for label, _ in scenes]},
        'input_fingerprints': [{'path': p, 'bytes': (ROOT/p).stat().st_size,
                                'sha256': hashlib.sha256((ROOT/p).read_bytes()).hexdigest()}
                               for p in sources],
        'limits': ['No independent tester/effector execution', 'No shader or device test',
                   'No performance/product PASS', 'No physical trajectory or material identification',
                   'Candidate perceptual thresholds need calibration',
                   'Raw filesystem hashes include current shared working-tree bytes'],
    }
    (HERE/'design-T356-single-sand-evidence-v1.json').write_text(
        json.dumps(evidence, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'checks': checks, 'contrast': contrast, 'grain_rms': rms,
                      'contact': detail_stats, 'figure': str(image_path)}, ensure_ascii=False))


if __name__ == '__main__':
    main()
