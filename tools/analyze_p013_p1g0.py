#!/usr/bin/env python3
"""P-013 P1-G0 platform probe analysis: pixel verdicts (sRGB / z-order) + log verdicts
(input pass-through / surface lifecycle / renderer frame-submission).
Usage: python analyze_p013_p1g0.py <art_dir>
"""
import sys, os, re, json, collections
import numpy as np
from PIL import Image

# hello 模块的恒定清屏色 glClearColor(0.18,0.20,0.22,1.0) 的 8bit 直写值
GLES_AUTHORED = (46, 51, 56)
# 若帧缓冲被当线性处理、显示端做 linear->sRGB 转换，同一数值应呈现为
GLES_IF_LINEAR = (118, 124, 129)


def openany(p):
    raw = open(p, 'rb').read()
    for enc in ('utf-16', 'utf-8'):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            continue
    return raw.decode('utf-8', errors='replace')


def load(path):
    return np.asarray(Image.open(path).convert('RGB')).astype(np.int32)


def shot_verdict(path, label):
    if not os.path.exists(path):
        print('   [%s] 缺截图' % label)
        return
    img = load(path)
    H, W, _ = img.shape
    cy = H // 2
    gles = tuple(int(v) for v in img[cy, W // 2])
    bar = tuple(int(v) for v in img[cy, 6])
    outside = tuple(int(v) for v in img[H - 20, W // 2])
    def near(a, b, tol=1):
        return all(abs(int(x) - int(y)) <= tol for x, y in zip(a, b))
    srgb = '一致(sRGB 直写，±1/通道)' if near(gles, GLES_AUTHORED) else (
        '线性→sRGB 转换后(色空间错配)' if near(gles, GLES_IF_LINEAR, 6) else '其他值')
    zorder = 'GLES 覆盖应用前景(XComponent 恒在上层)' if bar == gles else (
        '应用前景可见(在 GLES 之上)' if bar[0] > 180 and bar[2] > 180 else '不可判')
    print('   [%s] GLES区=%s 洋红条=%s 页面底=%s' % (label, gles, bar, outside))
    print('        ②sRGB: %s   ③z序: %s' % (srgb, zorder))


def log_input(path, label):
    if not os.path.exists(path):
        print('   [%s] 缺日志' % label)
        return
    t = openany(path)
    ins = re.findall(r'WS_EVENT p013_g0_input (\{.*?\})\s*$', t, re.M)
    downs = []
    for s in ins:
        try:
            downs.append(json.loads(s).get('down'))
        except Exception:
            pass
    hellos = re.findall(r'WS_EVENT p013_g0_hello (\{.*?\})\s*$', t, re.M)
    last = ''
    if hellos:
        try:
            d = json.loads(hellos[-1])
            last = 'frames=%s nativeFrames=%s ready=%s' % (d.get('frames'), d.get('nativeFrames'), d.get('ready'))
        except Exception:
            pass
    print('   [%s] 下层Canvas Down 计数=%d 序列=%s  %s' % (label, len(downs), downs, last))


def log_surface(path):
    if not os.path.exists(path):
        print('   [surface 逐轮 dump] 缺日志')
        return
    lines = [l for l in openany(path).splitlines() if l.strip()]
    uniq = list(dict.fromkeys(lines))
    c = collections.Counter()
    for l in uniq:
        for k in ['OnSurfaceCreated', 'OnSurfaceChanged', 'OnSurfaceDestroyed', 'EGL init ok', 'EGL destroy']:
            if k in l:
                c[k] += 1
                break
    print('   [surface 逐轮 dump] 去重后 %d 行（原始 %d）：%s' % (len(uniq), len(lines), dict(c)))


def log_renderer(path):
    if not os.path.exists(path):
        print('   [renderer 路径] 缺日志')
        return
    t = openany(path)
    de = re.findall(r'WS_EVENT p013_g0_draw_error code=(\d+)', t)
    tel = re.findall(r'WS_EVENT p013_g0_telemetry (\{.*?\})\s*$', t, re.M)
    init = re.findall(r'WS_EVENT p013_g0_init (\{.*?\})\s*$', t, re.M)
    if init:
        try:
            d = json.loads(init[0])
            print('   [renderer] init: code=%s state=%s ownerBound=%s surfaceEpoch=%s' % (
                d.get('code'), d.get('state'), d.get('ownerBound'), d.get('surfaceEpoch')))
        except Exception:
            pass
    print('   [renderer] 帧提交拒绝: %d 次，错误码分布=%s' % (len(de), dict(collections.Counter(de))))
    if tel:
        try:
            d = json.loads(tel[-1])
            print('   [renderer] 末条遥测: code=%s errorStage=%s T_dataHash=%s surfaceCreate=%s contextCreate=%s swapMs=%s' % (
                d.get('code'), d.get('errorStage'), d.get('T_dataHash'), d.get('surfaceCreateCount'),
                d.get('contextCreateCount'), d.get('swapMs')))
        except Exception:
            pass


def main():
    art = sys.argv[1] if len(sys.argv) > 1 else '.'
    print('=' * 78)
    print('P-013 P1-G0 五项平台探针分析（%s）' % art)
    print('--- ②sRGB / ③z序：像素判读 ---')
    for n, l in [('P1-hello-hitnone', 'hello hit=none'),
                  ('P2-hello-hitdefault-after-swipe', 'hello hit=default'),
                  ('P3-hello-hittransparent-after-swipe', 'hello hit=transparent'),
                  ('P4-hello-surface10bg', 'hello 6x bg/fg'),
                  ('P5-renderer-diagon', 'renderer diag=ON')]:
        shot_verdict(os.path.join(art, n + '.jpeg'), l)
    print('--- ⑤输入透传：各 hitTest 档的触摸计数 ---')
    for n, l in [('P1-hello-hitnone-g0.txt', 'hit=none'),
                 ('P2-hello-hitdefault-g0.txt', 'hit=default'),
                 ('P3-hello-hittransparent-g0.txt', 'hit=transparent')]:
        log_input(os.path.join(art, n), l)
    print('--- ①surface 生命周期 ---')
    log_surface(os.path.join(art, 'P4b-surface-cycles.txt'))
    print('--- ④透明 / renderer 帧提交 ---')
    log_renderer(os.path.join(art, 'P5-renderer-diagon-g0.txt'))


if __name__ == '__main__':
    main()
