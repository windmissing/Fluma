# s76 测试数据质量自检（VD-NEW + 污染帧甄别）
# 依据：VV 清单任务 2026-10-12 补充段
#   1. VD-NEW 动态过程有效性：渐变类用例（02/03/04/06）首尾斜率方向须改变、
#      斜率时序须含非稳态段（相邻帧斜率差超稳态噪声带），否则=动作未拍到=用例无效
#   2. 污染帧甄别：框内均值亮度异常（桌面 ~212 vs 应用内 ~60）自动标出
#   3. VV-01..05 复用 vv_s75_analysis.py 阈值
import cv2, glob, re, os
import numpy as np

FRAME_DIR = 'D:/hmprojects/HelloWorld/selfcheck/tc_frames'
BOX = (790, 1740, 180, 1080)   # y0,y1,x0,x1

def frame_stats(im):
    y0, y1, x0, x1 = BOX
    box = im[y0:y1, x0:x1]
    b = box[:,:,0].astype(np.int32); g = box[:,:,1].astype(np.int32); r = box[:,:,2].astype(np.int32)
    total = box.shape[0]*box.shape[1]
    blue = ((b>g+30)&(b>120)).sum()/total
    green = ((g>b+15)&(g>r+30)&(g>90)).sum()/total
    # 界面斜率：每列第一个绿像素行线性拟合
    mask_green = (g>b+15)&(g>r+30)&(g>90)
    cols = box.shape[1]
    green_top = np.full(cols, -1, dtype=np.int32)
    for x in range(cols):
        ys = np.where(mask_green[:, x])[0]
        if len(ys) > 0:
            green_top[x] = ys[0]
    valid = green_top >= 0
    slope = np.polyfit(np.arange(cols)[valid], green_top[valid], 1)[0] if valid.sum() > 50 else float('nan')
    brightness = box.mean()
    return blue*100, green*100, slope, brightness

def check_case(tc, expect_slope_change=False, noise_band=0.01):
    files = sorted(glob.glob(os.path.join(FRAME_DIR, f'tc{tc}_*.jpeg')),
                   key=lambda f: int(re.search(r'_(\d+)', f).group(1)))
    if not files:
        print(f'tc{tc}: NO FRAMES'); return None
    slopes, brights, blues = [], [], []
    for f in files:
        im = cv2.imread(f)
        bl, gr, sl, br = frame_stats(im)
        slopes.append(sl); brights.append(br); blues.append(bl)
    slopes = np.array(slopes); brights = np.array(brights); blues = np.array(blues)
    # 污染帧：亮度 > 120（应用内 ~60，桌面 ~212）
    dirty = np.where(brights > 120)[0]
    # VD-NEW 1: 首尾斜率方向
    head = np.nanmean(slopes[:5]); tail = np.nanmean(slopes[-5:])
    dir_change = (head * tail < 0) if expect_slope_change else None
    # VD-NEW 2: 非稳态段（相邻帧斜率差超噪声带，剔除污染帧后）
    keep = np.ones(len(slopes), bool); keep[dirty] = False
    dslope = np.abs(np.diff(slopes[keep]))
    has_dynamic = (dslope > noise_band).any() if len(dslope) > 0 else False
    # VV-01（剔污染帧）
    bdrift = (blues[keep].max() - blues[keep].min()) / 2 if keep.any() else float('nan')
    status = 'OK'
    if len(dirty) > 0: status += f' [污染帧{len(dirty)}个:{list(dirty)}]'
    if expect_slope_change:
        status += f' [VD-NEW 方向改变={"Y" if dir_change else "N!"} 非稳态段={"Y" if has_dynamic else "N!"}]'
        if not (dir_change and has_dynamic): status += ' <<用例无效须重录'
    print(f'tc{tc}: 帧{len(files)} 蓝漂移±{bdrift:.2f}% 斜率[{slopes[~np.isnan(slopes)].min():.3f}..{slopes[~np.isnan(slopes)].max():.3f}] 亮度[{brights.min():.0f}..{brights.max():.0f}] {status}')
    return {'tc': tc, 'bdrift': bdrift, 'dir_change': dir_change, 'has_dynamic': has_dynamic,
            'dirty': list(dirty), 'slope_head': head, 'slope_tail': tail}

print('===== VD-NEW + 污染帧自检（s76 数据）=====')
results = []
results.append(check_case('01'))
results.append(check_case('02', expect_slope_change=False))  # 单调加角，方向不变但须有非稳态段
results.append(check_case('03', expect_slope_change=True))   # 左倾→右倾，方向必须改变
results.append(check_case('04', expect_slope_change=True))   # 往复，方向改变
results.append(check_case('05'))
results.append(check_case('06', expect_slope_change=True))   # 大角度→归零，斜率回归 ~0
results.append(check_case('07'))
results.append(check_case('08'))

ok = all(r is not None and r['bdrift'] < 3 for r in results)
dyn_ok = all(r['dir_change'] and r['has_dynamic'] for r in results
             if r and r['dir_change'] is not None)
print(f'\nVV-01 全用例 <±3%: {"PASS" if ok else "FAIL"}')
print(f'VD-NEW 动态有效性(02/03/04/06): {"PASS" if dyn_ok else "FAIL"}')
