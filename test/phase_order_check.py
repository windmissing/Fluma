# 相序检查（TC-09/10 倒立场景专用判定）
# 方案定义：稳态倒立帧要求绿质心行号 < 蓝质心行号（绿在上方，重相到达重力反方向顶部）
# 正立帧要求蓝质心行号 < 绿质心行号（蓝上绿下）
# 质心行号 = 该相所有像素的 y 均值（框内物理像素坐标）
import cv2, glob, re, os, sys
import numpy as np

BOX = (790, 1740, 180, 1080)  # y0,y1,x0,x1

def phase_centroids(im):
    y0, y1, x0, x1 = BOX
    box = im[y0:y1, x0:x1]
    b = box[:,:,0].astype(np.int32); g = box[:,:,1].astype(np.int32); r = box[:,:,2].astype(np.int32)
    mask_blue = (b > g + 30) & (b > 120)
    mask_green = (g > b + 15) & (g > r + 30) & (g > 90)
    out = {}
    for name, m in [('blue', mask_blue), ('green', mask_green)]:
        if m.sum() > 100:
            ys, _ = np.where(m)
            out[name] = ys.mean()
        else:
            out[name] = float('nan')
    return out

def check_phase_order(files, expect, label):
    """expect: 'inverted'=绿上蓝下, 'upright'=蓝上绿下, 'both'=逐帧按给定序列"""
    rows = []
    for f in files:
        im = cv2.imread(f)
        c = phase_centroids(im)
        idx = int(re.search(r'_(\d+)', f).group(1)) if re.search(r'_(\d+)', f) else len(rows)
        rows.append((idx, c['blue'], c['green']))
    if expect == 'inverted':
        bad = [(i, b, g) for i, b, g in rows if not (g < b)]
        ok = len(bad) <= max(1, len(rows) // 10)  # 允许 10% 过渡帧
        print(f'{label}: {len(rows)} 帧, 相序违反 {len(bad)} 帧 → {"PASS" if ok else "FAIL"}')
        if not ok and bad:
            i, b, g = bad[0]
            print(f'  首个违反帧 f{i:03d}: blue_y={b:.0f} green_y={g:.0f}（绿应在蓝上方）')
    elif expect == 'upright':
        bad = [(i, b, g) for i, b, g in rows if not (b < g)]
        ok = len(bad) <= max(1, len(rows) // 10)
        print(f'{label}: {len(rows)} 帧, 相序违反 {len(bad)} 帧 → {"PASS" if ok else "FAIL"}')
        if not ok and bad:
            i, b, g = bad[0]
            print(f'  首个违反帧 f{i:03d}: blue_y={b:.0f} green_y={g:.0f}（蓝应在绿上方）')
    return rows

if __name__ == '__main__':
    FRAME_DIR = 'D:/hmprojects/HelloWorld/selfcheck/tc_frames'
    if len(sys.argv) >= 3:
        tc = sys.argv[1]
        expect = sys.argv[2]  # inverted / upright
        files = sorted(glob.glob(os.path.join(FRAME_DIR, f'tc{tc}_*.jpeg')),
                       key=lambda f: int(re.search(r'_(\d+)', f).group(1)))
        check_phase_order(files, expect, f'tc{tc}')
    else:
        print('用法: python phase_order_check.py <tcNN> <inverted|upright>')
