# VV-01..06 硬指标分析：70 帧连拍
# 先做竖直 BGR 切片标定颜色阈值，再逐帧统计
import cv2, glob, re, hashlib, os
import numpy as np

FRAME_DIR = 'D:/hmprojects/HelloWorld/test/frames_vv'
# 框区域：从 t0 截图目测，物理像素 1280x2832，框约 x 170..1090, y 790..1740
BOX = (790, 1740, 180, 1080)  # y0,y1,x0,x1

files = sorted(glob.glob(os.path.join(FRAME_DIR, 'f_*.jpeg')),
               key=lambda f: int(re.search(r'f_(\d+)', f).group(1)))
print('frames:', len(files))

# ===== 标定：取第一帧竖直 BGR 切片 =====
im0 = cv2.imread(files[0])
y0, y1, x0, x1 = BOX
col = im0[y0:y1, x0 + (x1 - x0)//2]  # 中间列
print('=== BGR vertical slice (middle column, every 40px) ===')
bh = y1 - y0
for y in range(0, bh, 40):
    b, g, r = col[y]
    print(f'y={y:4d} B={b:3d} G={g:3d} R={r:3d}')

# ===== 标定结论（写入注释）=====
# 暗背景:   B~26-36, G~14-16, R~14-15（深色）
# 蓝相:     B>140, G 90-150, R<60（蓝系）
# 绿相:     B<100, G>110, R 50-90（绿系）
# 混合带:   y=800 附近 B=67 G=132 —— 过渡色，按 B/G 关系归类
# 分类规则: g>b+15 且 g>r+30 → 绿; b>g+30 且 b>120 → 蓝; 其余 → 过渡(不计入分相)
# USB 弹窗剔除: 中央区(高度35%-62%)白像素(R,G,B均>230) > 8万

def classify_frame(im):
    y0, y1, x0, x1 = BOX
    box = im[y0:y1, x0:x1]
    b = box[:, :, 0].astype(np.int32)
    g = box[:, :, 1].astype(np.int32)
    r = box[:, :, 2].astype(np.int32)
    total = box.shape[0] * box.shape[1]
    blue = ((b > g + 30) & (b > 120)).sum()
    green = ((g > b + 15) & (g > r + 30) & (g > 90)).sum()
    # 中央白像素（弹窗检测）
    cy0 = int(0.35 * 2832); cy1 = int(0.62 * 2832)
    cx0 = int(0.15 * 1280); cx1 = int(0.85 * 1280)
    center = im[cy0:cy1, cx0:cx1]
    white = ((center[:, :, 0] > 230) & (center[:, :, 1] > 230) & (center[:, :, 2] > 230)).sum()
    # 液面高度：轻相（蓝）顶面平均 y（在框内找最上方的分相像素）
    # 蓝顶=每列第一个蓝像素行; 用全部列
    mask_blue = ((b > g + 30) & (b > 120))
    mask_green = ((g > b + 15) & (g > r + 30) & (g > 90))
    # 界面：绿相顶面逐列
    cols = box.shape[1]
    green_top = np.full(cols, -1, dtype=np.int32)
    for x in range(cols):
        ys = np.where(mask_green[:, x])[0]
        if len(ys) > 0:
            green_top[x] = ys[0]
    valid = green_top >= 0
    interface_y = green_top[valid].mean() if valid.sum() > 50 else float('nan')
    # 界面斜率: 对 valid 列做线性拟合（x, green_top）
    if valid.sum() > 50:
        xs = np.arange(cols)[valid]
        slope = np.polyfit(xs, green_top[valid], 1)[0]
    else:
        slope = float('nan')
    # 蓝顶面（液面）
    blue_top = np.full(cols, -1, dtype=np.int32)
    for x in range(cols):
        ys = np.where(mask_blue[:, x])[0]
        if len(ys) > 0:
            blue_top[x] = ys[0]
    vblue = blue_top >= 0
    surface_y = blue_top[vblue].mean() if vblue.sum() > 50 else float('nan')
    # 绿质心 x
    if mask_green.sum() > 100:
        ys, xs = np.where(mask_green)
        cx = xs.mean()
    else:
        cx = float('nan')
    return blue, green, total, white, surface_y, interface_y, slope, cx

print('=== 标定切片结束，开始逐帧统计 ===')
rows = []
for f in files:
    idx = int(re.search(r'f_(\d+)', f).group(1))
    im = cv2.imread(f)
    blue, green, total, white, surface_y, interface_y, slope, cx = classify_frame(im)
    md5 = hashlib.md5(open(f, 'rb').read()).hexdigest()[:8]
    popup = white > 80000
    rows.append((idx, blue/total, green/total, popup, surface_y, interface_y, slope, cx, md5))

print(f"{'frm':>4} {'blue%':>6} {'green%':>7} {'pop':>3} {'surf_y':>7} {'intf_y':>7} {'slope':>7} {'cx':>6} md5")
for idx, bp, gp, popup, sy, iy, sl, cx, md5 in rows:
    flag = 'POPUP' if popup else ''
    print(f"{idx:4d} {bp*100:6.2f} {gp*100:7.2f} {'Y' if popup else 'N':>3} {sy:7.1f} {iy:7.1f} {sl:7.3f} {cx:6.1f} {md5} {flag}")

# 汇总判定
valid_rows = [r for r in rows if not r[3]]
print(f'\n总帧数={len(rows)}, 弹窗剔除={len(rows)-len(valid_rows)}')
bps = [r[1] for r in valid_rows]; gps = [r[2] for r in valid_rows]
print(f'VV-01 blue% min={min(bps)*100:.2f} max={max(bps)*100:.2f} drift=±{(max(bps)-min(bps))/2*100:.2f}%')
print(f'VV-01 green% min={min(gps)*100:.2f} max={max(gps)*100:.2f} drift=±{(max(gps)-min(gps))/2*100:.2f}%')
md5s = set(r[8] for r in valid_rows)
print(f'VV-05 唯一帧数={len(md5s)}/{len(valid_rows)}')
surfs = [r[4] for r in valid_rows if not np.isnan(r[4])]
print(f'VV-02 surf_y min={min(surfs):.1f} max={max(surfs):.1f} jump={max(surfs)-min(surfs):.1f}')
sls = [r[6] for r in valid_rows if not np.isnan(r[6])]
print(f'VV-03 slope min={min(sls):.3f} max={max(sls):.3f}')
cxs = [r[7] for r in valid_rows if not np.isnan(r[7])]
print(f'VV-04 cx min={min(cxs):.1f} max={max(cxs):.1f}')
