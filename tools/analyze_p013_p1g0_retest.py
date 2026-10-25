#!/usr/bin/env python3
"""P-013 P1-G0 补证复测 T42 分析器：五项闭环判读（T41 帧长修复后）
   ①帧提交 ②sRGB（A/B/C 三层） ③三层 z 序 ④透明（二值 alpha） ⑤输入透传
用法: python analyze_p013_p1g0_retest.py <art_dir>
输入: tools/p013_p1g0_retest.ps1 采集的 R0-R8 日志与截图。
判读口径: docs/architecture-P013-p1-g0-composition-revision-v1.md §3/§4/§6。

T42 修正（相对 T35 版）：
- openany 改 BOM 检测 / utf-8 优先（utf-16 盲猜会把 UTF-8 流解出乱码致正则失配）
- frame_admission_verdict: errorStage 'NONE'/'null'/None 均视作无错（T35 误报 FAIL）
- shot_pixel_verdict: 从 p013_g0_box 事件取盒坐标与 surface 尺寸，
  屏高与 surface 高差=状态栏偏移 dy；GL 坐标自底向上 → 条纹在盒区屏幕坐标下半；
  五带采样对 sRGB 编码期望与直接字节期望分类；盒上半判透明；洋红穿盒判 z 序
- color_readback: hilog 单条 ~4096B 截断，取 glExtensions 前的头部 JSON 解析
"""
import sys, os, re, json, glob
import numpy as np
from PIL import Image

ART = sys.argv[1] if len(sys.argv) > 1 else '.'

# ── 解析期望（T33 §4.3 解析值，非冻结容差） ──
# 线性值 0.18 → 线性 8bit ≈ 46；sRGB 编码后 ≈ 118。线性 0.5 → 128 / 188。
ANALYTIC = {0.18: (46, 118), 0.5: (128, 188), 0.0031308: (1, 10), 0.0: (0, 0), 1.0: (255, 255)}
LIN_VALUES = [0.0, 0.0031308, 0.18, 0.5, 1.0]


def srgb8(v):
    return round((1.055 * v ** (1 / 2.4) - 0.055) * 255) if v > 0.0031308 else round(12.92 * v * 255)


def openany(p):
    raw = open(p, 'rb').read()
    if raw[:2] in (b'\xff\xfe', b'\xfe\xff'):
        return raw.decode('utf-16')
    if raw[:3] == b'\xef\xbb\xbf':
        return raw.decode('utf-8-sig')
    try:
        return raw.decode('utf-8')
    except UnicodeDecodeError:
        return raw.decode('utf-8', errors='replace')


def evlines(path):
    t = openany(path)
    out = {}
    for m in re.finditer(r'WS_EVENT (p013_g0_\w+|p013_tdata_boundary) (\{.*?\})\s*$', t, re.M):
        out.setdefault(m.group(1), []).append(m.group(2))
    # 非 JSON 事件（draw_error 等文本行）按出现次数计数
    out['_draw_error_codes'] = re.findall(r'WS_EVENT p013_g0_draw_error code=(\d+)', t)
    # p013_g0_box 文本行: box=(x,y,w,h) surface=WxH
    mb = re.search(r'WS_EVENT p013_g0_box box=\((\d+),(\d+),(\d+),(\d+)\) surface=(\d+)x(\d+)', t)
    if mb:
        out['_box'] = tuple(int(v) for v in mb.groups())
    # hilog 截断容忍：color_readback 头部 JSON（glExtensions 之前）
    for m in re.finditer(r'WS_EVENT p013_g0_color_readback (\{.*)$', t, re.M):
        js = m.group(1)
        cut = js.find('"glExtensions"')
        if cut > 0:
            js = js[:cut].rstrip().rstrip(',') + '}'
        d = jload(js)
        if d:
            out.setdefault('_readback_head', []).append(d)
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


def frame_admission_verdict(arm, tel, draw_codes, bounds, hello):
    """①帧提交判读（T42 版：T41 返修后）。返回 (verdict, detail_lines)。"""
    lines = []
    if hello:
        # hello 臂：恒定清屏色 surface，napi 面只有 drawFrame/isReady/getFrameCount
        last = hello[-1]
        ok = last.get('ok') is True and last.get('ready') is True and (last.get('nativeFrames') or 0) > 0
        lines.append('    hello: frames=%s native=%s ready=%s ok=%s' %
                     (last.get('frames'), last.get('nativeFrames'), last.get('ready'), last.get('ok')))
        return ('PASS' if ok else 'FAIL'), lines
    n_err = len(draw_codes)
    last = tel[-1] if tel else None
    swap_true = sum(1 for d in tel if d.get('swapSucceeded'))
    code0 = sum(1 for d in tel if d.get('code') == 0)
    has_hash = any(d.get('T_dataHash') for d in tel)
    sc = (last or {}).get('surfaceCreateCount'); cc = (last or {}).get('contextCreateCount')
    stage = (last or {}).get('errorStage')
    # 边界事件（T41 后 code=0 也打：frameId===1 时）
    b_ok = None
    if bounds:
        bs = [jload(x) for x in bounds]
        bs = [d for d in bs if d]
        if bs:
            b = bs[-1]
            b_ok = (b.get('length') == 512 and b.get('byteLength') == 2048 and
                    b.get('nativeByteLength') == 2048 and b.get('nativeBufferBytes') == 2048 and
                    b.get('errorStage') in (None, 'null', 'NONE'))
            lines.append('    tdata_boundary: length=%s byteLength=%s nativeByteLength=%s nativeBufferBytes=%s errorStage=%s → %s' %
                         (b.get('length'), b.get('byteLength'), b.get('nativeByteLength'),
                          b.get('nativeBufferBytes'), b.get('errorStage'), 'OK' if b_ok else 'MISMATCH'))
    if tel:
        lines.append('    遥测: n=%d code0=%d swapTrue=%d T_dataHash非零=%s surfaceCreate=%s ctxCreate=%s errorStage=%s' %
                     (len(tel), code0, swap_true, has_hash, sc, cc, stage))
    if n_err:
        from collections import Counter
        lines.append('    draw_error 次数=%d 码分布=%s' % (n_err, dict(Counter(draw_codes))))
    # 判定
    if not tel and not bounds:
        return 'UNPROVEN', lines + ['    无遥测无边界事件：采集缺失']
    if n_err == 0 and code0 > 0 and swap_true > 0 and has_hash and (sc or 0) >= 1 and (cc or 0) >= 1:
        v = 'PASS'
    elif n_err > 0 and code0 == 0:
        v = 'FAIL'
    elif code0 > 0 and swap_true == 0:
        v = 'FAIL'
    elif not has_hash:
        v = 'FAIL'
    elif (sc or 0) < 1 or (cc or 0) < 1:
        v = 'FAIL'
    else:
        v = 'UNPROVEN'
    if b_ok is False:
        v = 'FAIL'
    return v, lines


def shot_pixel_verdict(path, label, box, kind):
    """③z 序 + ④透明 + ②C 层 的截图判读。
    box=(bx,by,bw,bh,surfW,surfH)；kind: 'texture'|'surface'|'hello'。
    TEXTURE 布局: 全屏棋盘底 + 盒区(GL 底向上 → 屏幕下半显 5 条纹, 上半透明) + 洋红标记。
    """
    if not os.path.exists(path):
        print('   [%s] 缺截图' % label); return None
    img = load(path); H, W, _ = img.shape
    out = {'label': label}
    if box is None:
        print('   [%s] 无盒坐标事件，跳过像素判读' % label); return None
    bx, by, bw, bh, sw, sh = box
    dy = H - sh  # 状态栏偏移（屏高-surface 高）
    # 盒区屏幕坐标。GL 坐标自底向上：u_box.y=sh-by-bh，盒 GL y∈[sh-by-bh, sh-by]
    # → 屏 y = dy + sh - y_gl，故盒顶(屏)=dy+by，盒底(屏)=dy+by+bh
    top = dy + by                 # 盒顶（屏）
    bot = dy + by + bh            # 盒底（屏）
    # ③z 序: 洋红横线穿盒（盒 x 范围内逐行扫）
    mag_row = None
    for yy in range(top, bot, 3):
        seg = img[yy, bx:bx + bw]
        if ((seg[:, 0] > 180) & (seg[:, 1] < 90) & (seg[:, 2] > 180)).sum() > bw * 0.6:
            mag_row = yy; break
    mag_left = any(is_magenta(px(img, 12, int(H * f))) for f in (0.2, 0.4, 0.6, 0.8))
    if mag_row and mag_left:
        z_v = 'PASS 洋红线穿盒可见（y=%d, fg 在 GLES 之上）' % mag_row
    elif mag_left:
        z_v = 'FAIL 洋红仅盒外可见，盒区被 GLES 覆盖'
    else:
        z_v = 'FAIL 洋红条整体不可见'
    if kind == 'texture':
        if 'diagoff' in label:
            # R5: diagnosticEnabled=false → 探针按设计不画条纹（仅清透明），C 层无可判对象
            y_up = dy + by + int(bh * 0.25)
            up = [px(img, bx + int(bw * f), y_up) for f in (0.1, 0.3, 0.5, 0.7, 0.9)]
            n_cy = sum(1 for c in up if is_cyan(c)); n_yl = sum(1 for c in up if is_yellow(c))
            a_v = ('PASS 盒区全透明（diag=off 不画条纹，棋盘透出）' if n_cy + n_yl >= 4 else
                   'UNPROVEN 盒上半=%s' % (up,))
            print('   [%s] diag=off：探针按设计不画条纹；盒上半=%s' % (label, up[2]))
            print('        ②C层: N/A（diagnosticEnabled=false，无条纹可采）')
            print('        ④透明: %s  ③z序: %s' % (a_v, z_v))
            out.update({'srgbC': 'N/A', 'alpha': a_v.split()[0], 'zorder': z_v.split()[0]})
            return out
        # ②C 层: 五条纹。探针在 GL y = u_box.y + bh*0.25 处采样（uv.y=0.25<0.5 条纹区内）
        #   → 屏 y = dy + by + bh*0.75
        y_stripe = dy + by + int(bh * 0.75)
        bands = []
        for i in range(5):
            cx = bx + int((i + 0.5) * bw / 5)
            bands.append(px(img, cx, y_stripe))
        exp_s = [srgb8(v) for v in LIN_VALUES]
        exp_d = [round(v * 255) for v in LIN_VALUES]
        n_s = sum(1 for c, e in zip(bands, exp_s) if max(abs(c[0] - e), abs(c[1] - e), abs(c[2] - e)) <= 2)
        n_d = sum(1 for c, e in zip(bands, exp_d) if max(abs(c[0] - e), abs(c[1] - e), abs(c[2] - e)) <= 2)
        if n_s == 5:
            c_v = 'PASS 五带=sRGB 编码期望 %s（线性写入经 sRGB 写控制）' % exp_s
        elif n_d == 5:
            c_v = 'FAIL 五带=直接字节 %s（线性值未编码，framebuffer 被当线性）' % exp_d
        else:
            c_v = 'UNPROVEN 五带=%s（sRGB 期望 %s / 直接 %s）' % ([c[0] for c in bands], exp_s, exp_d)
        # ④透明: 盒上半（屏 y=dy+by .. dy+by+bh/2，即 GL uv.y>=0.5 区）应透出棋盘
        y_up = dy + by + int(bh * 0.25)
        up = [px(img, bx + int(bw * f), y_up) for f in (0.1, 0.3, 0.5, 0.7, 0.9)]
        n_cy = sum(1 for c in up if is_cyan(c)); n_yl = sum(1 for c in up if is_yellow(c))
        if n_cy + n_yl >= 4:
            a_v = 'PASS 盒上半棋盘透出（alpha=0 区正确合成）'
        elif sum(1 for c in up if is_black(c)) >= 4:
            a_v = 'FAIL 盒上半纯黑（alpha 未合成）'
        else:
            a_v = 'UNPROVEN 盒上半=%s' % (up,)
        print('   [%s] 五带=%s 盒上半=%s' % (label, [c[0] for c in bands], up[2]))
        print('        ②C层: %s' % c_v)
        print('        ④透明: %s  ③z序: %s' % (a_v, z_v))
        out.update({'srgbC': c_v.split()[0], 'alpha': a_v.split()[0], 'zorder': z_v.split()[0]})
    else:
        # SURFACE / hello 臂：透明不适用（surface 独立层/不透明清屏）
        sample = px(img, bx + bw // 2, (top + bot) // 2)
        print('   [%s] 盒区中心=%s ③z序: %s' % (label, sample, z_v))
        out.update({'alpha': 'N/A', 'zorder': z_v.split()[0]})
    return out


def main():
    print('== P1-G0 补证复测 T42 分析: %s' % os.path.abspath(ART))
    logs = sorted(glob.glob(os.path.join(ART, 'R*-g0.txt')))
    canvas_w = 0; box = None
    for p in logs:
        ev = evlines(p)
        if not canvas_w:
            for s in ev.get('p013_g0_canvas', []):
                d = jload(s)
                if d and d.get('w'):
                    canvas_w = int(d['w']); break
        if ev.get('_box'):
            box = ev['_box']; break
    print('-- canvas 逻辑宽: %s；盒坐标: %s' % (canvas_w, box))

    verdicts = {}
    print('\n-- ①帧提交 / ②sRGB 收据(A/B) / ⑤输入（逐臂日志判读）')
    for p in logs:
        arm = os.path.basename(p).replace('-g0.txt', '')
        ev = evlines(p)
        tel = [jload(s) for s in ev.get('p013_g0_telemetry', [])]
        tel = [d for d in tel if d]
        draw_codes = ev.get('_draw_error_codes', [])
        comps = [jload(s) for s in ev.get('p013_g0_composition', [])]
        comps = [d for d in comps if d]
        reads = ev.get('_readback_head', [])
        ins = [jload(s) for s in ev.get('p013_g0_input', [])]
        ins = [d for d in ins if d]
        hellos = [jload(s) for s in ev.get('p013_g0_hello', [])]
        hellos = [d for d in hellos if d]
        bounds = ev.get('p013_tdata_boundary', [])
        print('\n  [%s]' % arm)
        # hello 事件仅在本臂为 hello 臂时采信（防上一臂残留进程的尾事件混入，按 PID 不可分辨）
        hellos_use = hellos if 'hello' in arm else []
        fv, flines = frame_admission_verdict(arm, tel, draw_codes, bounds, hellos_use)
        for l in flines:
            print(l)
        print('    ⇒ ①帧提交: %s' % fv)
        verdicts.setdefault(arm, {})['frame'] = fv
        if tel:
            last = tel[-1]
            print('    遥测: surfaceCreate=%s surfaceDestroy=%s ctxCreate=%s ctxDestroy=%s ctxLoss=%s stale=%s eglErr=%s glErr=%s' %
                  (last.get('surfaceCreateCount'), last.get('surfaceDestroyCount'), last.get('contextCreateCount'),
                   last.get('contextDestroyCount'), last.get('contextLossCount'), last.get('staleLifecycleCount'),
                   last.get('eglError'), last.get('glError')))
        for d in comps:
            print('    合成收据(请求时): code=%s status=%s req=%s errStage=%s rbStage=%s' %
                  (d.get('code'), d.get('profileStatus'), d.get('requestedColorSpace'),
                   d.get('errorStage'), d.get('readbackStage')))
        if reads:
            d = reads[-1]
            print('    读回头部(帧后): act=%s attach=%s alphaSize=%s wctl=%s/%s drawFb=%s status=%s errStage=%s' %
                  (d.get('actualColorSpace'), d.get('attachmentEncoding'), d.get('eglAlphaSize'),
                   d.get('srgbWriteControlSupported'), d.get('srgbWriteEnabled'),
                   d.get('drawFramebuffer'), d.get('profileStatus'), d.get('errorStage')))
            verdicts[arm]['srgbAB'] = ('PASS' if (d.get('actualColorSpace') == 'SRGB' and
                                                  d.get('attachmentEncoding') == 'GL_SRGB' and
                                                  d.get('eglAlphaSize') == 8 and
                                                  d.get('srgbWriteEnabled') is True) else 'FAIL')
        if ins:
            print('    输入 Down=%d（hitMode=%s）' % (len(ins), ins[-1].get('hitMode')))
            verdicts[arm]['input'] = len(ins)

    print('\n-- ③z序 / ④透明 / ②C层（截图判读）')
    shots = sorted(glob.glob(os.path.join(ART, 'R*.jpeg')))
    for p in shots:
        label = os.path.basename(p).replace('.jpeg', '')
        if 'dyn-' in label and not label.endswith('dyn-1'):   # R7 连拍取首帧
            continue
        kind = 'hello' if 'hello' in label else ('surface' if label.startswith('R2-') else 'texture')
        r = shot_pixel_verdict(p, label, box, kind)
        if r:
            verdicts.setdefault(label, {}).update(r)

    print('\n-- 汇总')
    arms = sorted(verdicts.keys())
    frame_pass = all(verdicts[a].get('frame') == 'PASS' for a in arms if a.startswith('R') and '-g0' not in a and verdicts[a].get('frame'))
    print('  ①帧提交: %s' % ('PASS（全部臂 code=0/swap=true/draw_error=0）' if frame_pass else '见逐臂'))
    print('  ②sRGB : A/B 见各臂读回头部；C 层见五带判读')
    print('  ③z序  : %s' % ', '.join('%s=%s' % (a, verdicts[a].get('zorder', '?')) for a in arms if verdicts[a].get('zorder')))
    print('  ④透明  : %s' % ', '.join('%s=%s' % (a, verdicts[a].get('alpha', '?')) for a in arms if verdicts[a].get('alpha')))
    print('  ⑤输入  : %s' % ', '.join('%s=%s' % (a, verdicts[a].get('input', '?')) for a in arms if verdicts[a].get('input') is not None))
    print('\n-- ②sRGB 解析期望对照（T33 §4.3，MEASURED_NOT_CALIBRATED 容差未冻结）')
    print('   线性 0.18 → 线性8bit≈%d / sRGB8bit≈%d；线性 0.5 → %d / %d' %
          (ANALYTIC[0.18][0], ANALYTIC[0.18][1], ANALYTIC[0.5][0], ANALYTIC[0.5][1]))
    print('   判读规则: 读回值与请求 colorspace/attachment 编码一致才计该层证据；截图近似不补 A/B。')
    print('\nDONE')


if __name__ == '__main__':
    main()
