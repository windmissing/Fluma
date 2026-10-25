#!/usr/bin/env python3
"""P-013 P1-G0 补证复测（T35）分析器：五项闭环判读
   ①帧提交 ②sRGB（A/B/C 三层） ③三层 z 序 ④透明（二值 alpha） ⑤输入透传
用法: python analyze_p013_p1g0_retest.py <art_dir>
输入: tools/p013_p1g0_retest.ps1 采集的 R0-R7 日志与截图。
判读口径: docs/architecture-P013-p1-g0-composition-revision-v1.md §3/§4/§6。
"""
import sys, os, re, json, glob
import numpy as np
from PIL import Image

ART = sys.argv[1] if len(sys.argv) > 1 else '.'

# ── 解析期望（T33 §4.3 解析值，非冻结容差） ──
# 线性值 0.18 → 线性 8bit ≈ 46；sRGB 编码后 ≈ 118。线性 0.5 → 128 / 188。
ANALYTIC = {0.18: (46, 118), 0.5: (128, 188), 0.0031308: (1, 33), 0.0: (0, 0), 1.0: (255, 255)}


def openany(p):
    raw = open(p, 'rb').read()
    for enc in ('utf-16', 'utf-8'):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            continue
    return raw.decode('utf-8', errors='replace')


def evlines(path):
    t = openany(path)
    out = {}
    for m in re.finditer(r'WS_EVENT (p013_g0_\w+) (\{.*?\})\s*$', t, re.M):
        out.setdefault(m.group(1), []).append(m.group(2))
    # 非 JSON 事件（draw_error 等文本行）按出现次数计数
    out['_draw_error_codes'] = re.findall(r'WS_EVENT p013_g0_draw_error code=(\d+)', t)
    return out


def jload(s):
    try:
        return json.loads(s)
    except Exception:
        return None


def load(path):
    return np.asarray(Image.open(path).convert('RGB')).astype(np.int32)


def px(img, x, y):
    h, w, _ = img.shape
    x = min(max(int(x), 0), w - 1); y = min(max(int(y), 0), h - 1)
    return tuple(int(v) for v in img[y, x])


def is_magenta(c):
    return c[0] > 180 and c[1] < 90 and c[2] > 180


def is_cyan(c):
    return c[0] < 110 and c[1] > 170 and c[2] > 170


def is_yellow(c):
    return c[0] > 180 and c[1] > 140 and c[2] < 110


def is_black(c):
    return max(c) < 24


def shot_pixel_verdict(path, label, canvas_w):
    """③z 序 + ④透明 的 C 层判读。布局: 全屏棋盘底(24px 格) + 中央沙盒 + 左侧/中部洋红标记。
    帧提交被拒时 GLES 区无内容：透明=空纹理透出（vacuous），z 序=无内容可判（UNPROVEN）。"""
    if not os.path.exists(path):
        print('   [%s] 缺截图' % label); return None
    img = load(path); H, W, _ = img.shape
    sx = W / float(canvas_w) if canvas_w > 0 else 1.0
    # ④透明: 沙盒外（盒上方 y=0.12H）取 5 点棋盘采样
    oy = int(0.12 * H)
    outside = [px(img, int(W * f), oy) for f in (0.2, 0.35, 0.5, 0.65, 0.8)]
    n_cy = sum(1 for c in outside if is_cyan(c)); n_yl = sum(1 for c in outside if is_yellow(c))
    n_bk = sum(1 for c in outside if is_black(c))
    if n_cy + n_yl >= 3:
        alpha_v = 'PASS 棋盘透出（非沙区显透明合成）'
    elif n_bk >= 3:
        alpha_v = 'FAIL 非沙区纯黑（alpha 未合成或清屏不透明）'
    else:
        alpha_v = 'UNPROVEN 非沙区既非棋盘也非纯黑: %s' % (outside,)
    # 沙盒区（GLES 应渲染处）中心与四点: 有内容=非棋盘；无内容=棋盘透出
    bx0 = (W - 0.45 * W) / 2 * sx; by0 = 0.25 * H
    bx1 = bx0 + 0.45 * W * sx; by1 = by0 + 0.30 * H
    inside = [px(img, int((bx0 + bx1) / 2), int((by0 + by1) / 2)),
              px(img, int(bx0 + 0.1 * (bx1 - bx0)), int(by0 + 0.2 * (by1 - by0))),
              px(img, int(bx1 - 0.1 * (bx1 - bx0)), int(by1 - 0.2 * (by1 - by0)))]
    gles_has_content = not any(is_cyan(c) or is_yellow(c) for c in inside)
    # ③z 序: 逐行扫描找洋红横线（不假设 y=0.48H）
    rows_with_mag = []
    for yy in range(0, H, 6):
        if any(is_magenta(px(img, xx, yy)) for xx in range(int(0.20 * W), int(0.80 * W), 24)):
            rows_with_mag.append(yy)
    mag_rows = [yy for yy in rows_with_mag if by0 < yy < by1]
    mag_left = any(is_magenta(px(img, int(12 * sx), int(H * f))) for f in (0.2, 0.4, 0.6, 0.8))
    if not gles_has_content:
        z_v = 'UNPROVEN GLES 区无渲染内容（帧提交被拒），z 序无可判定对象'
    elif mag_rows and mag_left:
        z_v = 'PASS 前景洋红线穿沙盒可见（fg 在 GLES 之上）'
    elif mag_left and not mag_rows:
        z_v = 'FAIL 洋红仅盒外可见，沙盒区被 GLES 覆盖'
    elif not mag_left:
        z_v = 'FAIL 洋红条整体不可见'
    else:
        z_v = 'UNPROVEN'
    print('   [%s] 非沙区=%s 沙盒内=%s GLES有内容=%s' % (label, outside[2], inside[0], gles_has_content))
    print('        ④透明: %s  ③z序: %s' % (alpha_v, z_v))
    return {'alpha': alpha_v.split()[0], 'zorder': z_v.split()[0],
            'gles_has_content': gles_has_content}


def main():
    print('== P1-G0 补证复测分析: %s' % os.path.abspath(ART))
    logs = sorted(glob.glob(os.path.join(ART, 'R*-g0.txt')))
    canvas_w = 0
    for p in logs:
        for s in evlines(p).get('p013_g0_canvas', []):
            d = jload(s)
            if d and d.get('w'):
                canvas_w = int(d['w']); break
        if canvas_w:
            break
    print('-- canvas 逻辑宽: %s（截图缩放据此换算）' % canvas_w)

    print('\n-- ①帧提交 / ②sRGB 收据 / ⑤输入（逐臂日志判读）')
    for p in logs:
        arm = os.path.basename(p).replace('-g0.txt', '')
        ev = evlines(p)
        # 帧提交
        tel = [jload(s) for s in ev.get('p013_g0_telemetry', [])]
        tel = [d for d in tel if d]
        draw_codes = ev.get('_draw_error_codes', [])
        comps = [jload(s) for s in ev.get('p013_g0_composition', [])]
        comps = [d for d in comps if d]
        reads = [jload(s) for s in ev.get('p013_g0_color_readback', [])]
        reads = [d for d in reads if d]
        ins = [jload(s) for s in ev.get('p013_g0_input', [])]
        ins = [d for d in ins if d]
        hellos = [jload(s) for s in ev.get('p013_g0_hello', [])]
        hellos = [d for d in hellos if d]
        print('\n  [%s]' % arm)
        if hellos:
            last = hellos[-1]
            print('    hello: frames=%s native=%s ready=%s ok=%s' %
                  (last.get('frames'), last.get('nativeFrames'), last.get('ready'), last.get('ok')))
        if tel:
            last = tel[-1]
            print('    遥测: surfaceCreate=%s ctxCreate=%s code=%s swap=%s T_dataHash=%s eglErr=%s glErr=%s' %
                  (last.get('surfaceCreateCount'), last.get('contextCreateCount'), last.get('code'),
                   last.get('swapSucceeded'), last.get('T_dataHash'), last.get('eglError'), last.get('glError')))
        if draw_codes:
            from collections import Counter
            dist = Counter(draw_codes)
            print('    draw_error 次数=%d 码分布=%s' % (len(draw_codes), dict(dist)))
        else:
            print('    draw_error 次数=0')
        for d in comps:
            print('    合成收据: code=%s status=%s req=%s act=%s attach=%s alphaSize=%s wctl=%s/%s errStage=%s rbStage=%s rbStatus=%s' %
                  (d.get('code'), d.get('profileStatus'), d.get('requestedColorSpace'), d.get('actualColorSpace'),
                   d.get('attachmentEncoding'), d.get('eglAlphaSize'), d.get('srgbWriteControlSupported'),
                   d.get('srgbWriteEnabled'), d.get('errorStage'), d.get('readbackStage'), d.get('readbackStatus')))
        for d in reads:
            lin = d.get('linearRgba8') or []; sr = d.get('srgbRgba8') or []
            def fmt(a):
                return '[' + ','.join('%.0f' % v for v in a[:8]) + ('...' if len(a) > 8 else '') + ']'
            print('    读回: linearRgba8=%s srgbRgba8=%s status=%s' % (fmt(lin), fmt(sr), d.get('readbackStatus')))
        if ins:
            print('    输入 Down=%d（hitMode=%s）' % (len(ins), ins[-1].get('hitMode')))

    print('\n-- ③z序 / ④透明（截图 C 层判读）')
    for p in sorted(glob.glob(os.path.join(ART, 'R*.jpeg'))):
        label = os.path.basename(p).replace('.jpeg', '')
        shot_pixel_verdict(p, label, canvas_w)

    print('\n-- ②sRGB 解析期望对照（T33 §4.3，MEASURED_NOT_CALIBRATED 容差未冻结）')
    print('   线性 0.18 → 线性8bit≈%d / sRGB8bit≈%d；线性 0.5 → %d / %d' %
          (ANALYTIC[0.18][0], ANALYTIC[0.18][1], ANALYTIC[0.5][0], ANALYTIC[0.5][1]))
    print('   判读规则: 读回值与请求 colorspace/attachment 编码一致才计该层证据；截图近似不补 A/B。')
    print('\nDONE')


if __name__ == '__main__':
    main()
