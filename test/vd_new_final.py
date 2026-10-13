# VD-NEW 动态有效性终检（s76 测试模式数据）
import cv2, glob, re
import numpy as np

FRAME_DIR = 'D:/hmprojects/HelloWorld/selfcheck/tc_frames'

def slopes_of(files, mask='blue'):
    sls = []
    for f in files:
        im = cv2.imread(f)
        box = im[790:1740, 180:1080]
        b = box[:,:,0].astype(int); g = box[:,:,1].astype(int); r = box[:,:,2].astype(int)
        m = (b>g+30)&(b>120) if mask=='blue' else (g>b+15)&(g>r+30)&(g>90)
        cols = box.shape[1]
        t = np.full(cols, -1, dtype=np.int32)
        for x in range(cols):
            ys = np.where(m[:, x])[0]
            if len(ys) > 0: t[x] = ys[0]
        v = t >= 0
        sls.append(np.polyfit(np.arange(cols)[v], t[v], 1)[0] if v.sum() > 50 else float('nan'))
    return np.array(sls)

for tc, expect in [('02','monotonic'), ('03','dirchange'), ('04','dirchange'), ('06','converge')]:
    files = sorted(glob.glob(f'{FRAME_DIR}/tc{tc}_*.jpeg'), key=lambda f: int(re.search(r'_(\d+)', f).group(1)))
    s_surf = slopes_of(files, 'blue')
    head = np.nanmean(s_surf[:4]); tail = np.nanmean(s_surf[-4:])
    ds = np.abs(np.diff(s_surf[~np.isnan(s_surf)]))
    if expect == 'dirchange':
        ok = (head * tail < 0) or (abs(tail - head) > 0.1)
        crit = f'head={head:+.3f} tail={tail:+.3f}'
        verdict = 'PASS' if ok else 'FAIL'
    elif expect == 'converge':
        ok = abs(head) > 0.45 and abs(tail) < abs(head) * 0.75
        ratio = abs(tail)/abs(head)
        crit = f'head={head:+.3f} tail={tail:+.3f} 收敛比={ratio:.2f}'
        verdict = 'PASS' if ok else 'FAIL'
    else:
        ok = (ds > 0.01).any()
        crit = f'非稳态段={"Y" if ok else "N"}'
        verdict = 'PASS' if ok else 'FAIL'
    print(f'tc{tc}: {crit} -> VD-NEW {verdict}')
