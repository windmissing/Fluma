# s77p 倒立补测数据判定：TC-09/TC-10 帧分析（相序+VV-01+VD-NEW）
# 录制脚本 record_tc_all.py 已支持 09/10（want 参数版改用 aa start --ps）
import cv2, glob, re, os, sys
import numpy as np

FRAME_DIR = 'D:/hmprojects/HelloWorld/selfcheck/tc_frames'
BOX = (790, 1740, 180, 1080)

def frame_stats(im):
    y0, y1, x0, x1 = BOX
    box = im[y0:y1, x0:x1]
    b = box[:,:,0].astype(np.int32); g = box[:,:,1].astype(np.int32); r = box[:,:,2].astype(np.int32)
    total = box.shape[0]*box.shape[1]
    mb = (b>g+30)&(b>120); mg = (g>b+15)&(g>r+30)&(g>90)
    blue = mb.sum()/total; green = mg.sum()/total
    by = np.where(mb)[0].mean() if mb.sum()>100 else float('nan')
    gy = np.where(mg)[0].mean() if mg.sum()>100 else float('nan')
    bright = box.mean()
    return blue, green, by, gy, bright

def run(tc, expect_order):
    files = sorted(glob.glob(os.path.join(FRAME_DIR, f'tc{tc}_*.jpeg')),
                   key=lambda f: int(re.search(r'_(\d+)', f).group(1)))
    if not files:
        print(f'tc{tc}: NO FRAMES'); return
    blues, greens, orders, dirties = [], [], [], []
    for f in files:
        im = cv2.imread(f)
        bl, gr, by, gy, br = frame_stats(im)
        if br > 120: dirties.append(f); continue
        blues.append(bl*100); greens.append(gr*100)
        if not np.isnan(by):
            orders.append('绿上' if gy < by else '蓝上')
    bd = (max(blues)-min(blues))/2
    inv_frames = sum(1 for o in orders if o == '绿上')
    up_frames = sum(1 for o in orders if o == '蓝上')
    print(f'tc{tc}: {len(files)}帧 污染{len(dirties)} 蓝漂移±{bd:.2f}% 相序:绿上{inv_frames}帧/蓝上{up_frames}帧 → '
          f'VV-01 {"PASS" if bd < 3 else "FAIL"}, 相序{"PASS" if (expect_order=="绿上" and inv_frames > len(orders)*0.6) or (expect_order=="蓝上" and up_frames > len(orders)*0.6) else "FAIL"}')

if __name__ == '__main__':
    run(sys.argv[1] if len(sys.argv) > 1 else '09', sys.argv[2] if len(sys.argv) > 2 else '绿上')
