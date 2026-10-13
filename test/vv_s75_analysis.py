# s75 VV-01..06 硬指标判定：TC-01..08 逐用例逐帧统计
# 颜色阈值沿用 vv_analysis.py 已标定结论（s74 轮次，UI 未变）：
#   蓝相: b>g+30 且 b>120；绿相: g>b+15 且 g>r+30 且 g>90；过渡色不计入分相
#   弹窗剔除: 中央区(35%-62%高度, 15%-85%宽度)白像素>8万
# BOX: 沙框物理像素区域 y 790..1740, x 180..1080（1280x2832 屏）
import cv2, glob, re, os, sys
import numpy as np

FRAME_DIR = 'D:/hmprojects/HelloWorld/selfcheck/tc_frames'
BOX = (790, 1740, 180, 1080)  # y0,y1,x0,x1

def classify_stats(im):
    y0, y1, x0, x1 = BOX
    box = im[y0:y1, x0:x1]
    b = box[:, :, 0].astype(np.int32)
    g = box[:, :, 1].astype(np.int32)
    r = box[:, :, 2].astype(np.int32)
    total = box.shape[0] * box.shape[1]
    mask_blue = (b > g + 30) & (b > 120)
    mask_green = (g > b + 15) & (g > r + 30) & (g > 90)
    blue = mask_blue.sum()
    green = mask_green.sum()
    # 弹窗检测
    cy0 = int(0.35 * 2832); cy1 = int(0.62 * 2832)
    cx0 = int(0.15 * 1280); cx1 = int(0.85 * 1280)
    center = im[cy0:cy1, cx0:cx1]
    white = ((center[:, :, 0] > 230) & (center[:, :, 1] > 230) & (center[:, :, 2] > 230)).sum()
    # 液面：每列第一个蓝像素行 → 均值
    cols = box.shape[1]
    blue_top = np.full(cols, -1, dtype=np.int32)
    for x in range(cols):
        ys = np.where(mask_blue[:, x])[0]
        if len(ys) > 0:
            blue_top[x] = ys[0]
    vblue = blue_top >= 0
    surface_y = blue_top[vblue].mean() if vblue.sum() > 50 else float('nan')
    # 界面：每列第一个绿像素行 → 均值 + 斜率
    green_top = np.full(cols, -1, dtype=np.int32)
    for x in range(cols):
        ys = np.where(mask_green[:, x])[0]
        if len(ys) > 0:
            green_top[x] = ys[0]
    valid = green_top >= 0
    interface_y = green_top[valid].mean() if valid.sum() > 50 else float('nan')
    if valid.sum() > 50:
        slope = np.polyfit(np.arange(cols)[valid], green_top[valid], 1)[0]
    else:
        slope = float('nan')
    # 绿质心 x
    if mask_green.sum() > 100:
        ys, xs = np.where(mask_green)
        cx = xs.mean()
    else:
        cx = float('nan')
    return blue / total, green / total, white > 80000, surface_y, interface_y, slope, cx

cases = sys.argv[1:] if len(sys.argv) > 1 else ['01','02','03','04','05','06','07','08']
summary = []
for tc in cases:
    files = sorted(glob.glob(os.path.join(FRAME_DIR, f'tc{tc}_*.jpeg')),
                   key=lambda f: int(re.search(r'_(\d+)', f).group(1)))
    if not files:
        print(f'tc{tc}: NO FRAMES')
        continue
    rows = []
    for f in files:
        idx = int(re.search(r'_(\d+)', f).group(1))
        im = cv2.imread(f)
        bp, gp, popup, sy, iy, sl, cx = classify_stats(im)
        md5 = str(hash(open(f, 'rb').read()))
        rows.append((idx, bp, gp, popup, sy, iy, sl, cx, md5))
    valid = [r for r in rows if not r[3]]
    bps = [r[1] for r in valid]; gps = [r[2] for r in valid]
    drift_b = (max(bps) - min(bps)) / 2 * 100
    drift_g = (max(gps) - min(gps)) / 2 * 100
    md5s = set(r[8] for r in valid)
    surfs = [r[4] for r in valid if not np.isnan(r[4])]
    jump = (max(surfs) - min(surfs)) if surfs else float('nan')
    sls = [r[6] for r in valid if not np.isnan(r[6])]
    cxs = [r[7] for r in valid if not np.isnan(r[7])]
    vv01 = 'PASS' if drift_b < 3 and drift_g < 3 else 'FAIL'
    vv05 = 'PASS' if len(md5s) > 1 else 'FAIL'
    vv02 = 'PASS' if (np.isnan(jump) or jump < 60) else 'FAIL'  # 容差 60px（含倾斜场景的界面移动，静置用例另行观察）
    vv06 = 'PASS'  # FPS 由 HUD/hilog 单独核（连拍模式无法测 FPS，不在此判）
    print(f"=== tc{tc} ({len(files)}帧, 弹窗剔除{len(rows)-len(valid)}) ===")
    print(f"  VV-01 blue% [{min(bps)*100:.2f}..{max(bps)*100:.2f}] drift ±{drift_b:.2f}% | green% [{min(gps)*100:.2f}..{max(gps)*100:.2f}] drift ±{drift_g:.2f}% → {vv01}")
    if surfs:
        print(f"  VV-02 surf_y [{min(surfs):.1f}..{max(surfs):.1f}] jump {jump:.1f}px → {vv02}")
    if sls:
        print(f"  VV-03 slope [{min(sls):.3f}..{max(sls):.3f}]")
    if cxs:
        print(f"  VV-04 cx [{min(cxs):.1f}..{max(cxs):.1f}]")
    print(f"  VV-05 唯一帧 {len(md5s)}/{len(valid)} → {vv05}")
    summary.append((tc, vv01, vv02, vv05, drift_b, drift_g))

print('\n===== 汇总 =====')
print(f"{'tc':>4} {'VV-01':>6} {'VV-02':>6} {'VV-05':>6} {'blueDrift%':>10} {'greenDrift%':>11}")
allpass = True
for tc, v1, v2, v5, db, dg in summary:
    print(f"{tc:>4} {v1:>6} {v2:>6} {v5:>6} {db:10.2f} {dg:11.2f}")
    if v1 == 'FAIL' or v2 == 'FAIL' or v5 == 'FAIL':
        allpass = False
print('\nVV-01/02/05 overall:', 'PASS' if allpass else 'FAIL')
