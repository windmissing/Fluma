# s74 TC-01..08 正式测试：基于 tc_frames 219 帧逐用例判定
# 标定（tc01_000 中间列切片）：框约 y 1330-1980, x 168-1112
#   蓝相: B>140 且 B>G+60（主体 B~192,G~100）；高光带 B/G/R 全高
#   绿相: G>95 且 G>B+15（主体 G 100-160, B 40-90）
#   背景: B 20-46, G 10-26, R 10-25（暗）
#   白/弹窗: RGB 全 >200
import cv2, glob, os, hashlib
import numpy as np

FR = 'D:/hmprojects/HelloWorld/selfcheck/tc_frames'
BOX = (1330, 1980, 168, 1112)  # y0,y1,x0,x1

def load(tc):
    files = sorted(glob.glob(f'{FR}/{tc}_*.jpeg'))
    return files

def classify(im):
    y0,y1,x0,x1 = BOX
    box = im[y0:y1, x0:x1]
    b = box[:,:,0].astype(int); g = box[:,:,1].astype(int); r = box[:,:,2].astype(int)
    total = box.shape[0]*box.shape[1]
    blue = ((b>140)&(b>g+60)).sum()
    green = ((g>95)&(g>b+15)).sum()
    white = ((b>200)&(g>200)&(r>200)).sum()
    # 液面：每列蓝相顶端
    mb = (b>140)&(b>g+60)
    mg = (g>95)&(g>b+15)
    cols = box.shape[1]
    def topmean(mask):
        t = np.full(cols, np.nan)
        for x in range(cols):
            ys = np.where(mask[:,x])[0]
            if len(ys)>0: t[x] = ys[0]
        return np.nanmean(t) if np.sum(~np.isnan(t))>30 else np.nan, t
    surf, _ = topmean(mb)
    intf, tintf = topmean(mg)
    # 界面斜率：对绿顶拟合
    valid = ~np.isnan(tintf)
    slope = np.polyfit(np.arange(cols)[valid], tintf[valid], 1)[0] if valid.sum()>50 else np.nan
    # 绿质心 x
    if mg.sum()>100:
        ys,xs = np.where(mg); cx = xs.mean()
    else: cx = np.nan
    # 绿压蓝列比例
    bad=0; both=0
    for x in range(0,cols,4):
        yb = np.where(mb[:,x])[0]; yg = np.where(mg[:,x])[0]
        if len(yb)>0 and len(yg)>0:
            both+=1
            if yg[0]<yb[0]: bad+=1
    gpress = bad/max(1,both)
    return blue, green, total, white, surf, intf, slope, cx, gpress

def md5(f): return hashlib.md5(open(f,'rb').read()).hexdigest()[:10]

print('='*90)
print('TC-01..08 逐用例硬指标')
print('='*90)
results = {}
for tc in ['tc01','tc02','tc03','tc04','tc05','tc06','tc07','tc08']:
    files = load(tc)
    rows=[]
    for f in files:
        im = cv2.imread(f)
        blue,green,total,white,surf,intf,slope,cx,gp = classify(im)
        rows.append(dict(f=os.path.basename(f), bp=blue/total, gp=green/total,
                         white=white, surf=surf, intf=intf, slope=slope, cx=cx, gpress=gp,
                         md5=md5(f)))
    n = len(rows)
    uniq = len(set(r['md5'] for r in rows))
    bps = [r['bp'] for r in rows]; gps=[r['gp'] for r in rows]
    popup = sum(1 for r in rows if r['white']>80000)
    surfs=[r['surf'] for r in rows if not np.isnan(r['surf'])]
    sls=[r['slope'] for r in rows if not np.isnan(r['slope'])]
    cxs=[r['cx'] for r in rows if not np.isnan(r['cx'])]
    gpm = max(r['gp'] for r in rows)
    b_drift = (max(bps)-min(bps))/2*100
    results[tc]=dict(n=n,uniq=uniq,b0=bps[0]*100,b1=bps[-1]*100,bmin=min(bps)*100,bmax=max(bps)*100,
                     bdrift=b_drift, popup=popup, surf_min=min(surfs), surf_max=max(surfs),
                     sl_min=min(sls), sl_max=max(sls), cx_min=min(cxs), cx_max=max(cxs), gpress=gpm)
    print(f"\n{tc}: {n}帧 唯一md5={uniq} 弹窗={popup}")
    print(f"  蓝%: {bps[0]*100:.1f}→{bps[-1]*100:.1f} (min {min(bps)*100:.1f} max {max(bps)*100:.1f}) 漂移±{b_drift:.2f}%")
    print(f"  绿%: {gps[0]*100:.1f}→{gps[-1]*100:.1f}")
    print(f"  液面: {min(surfs):.0f}~{max(surfs):.0f}  斜率: {min(sls):.3f}~{max(sls):.3f}  质心x: {min(cxs):.0f}~{max(cxs):.0f}  绿压蓝峰值: {gpm*100:.0f}%")

print('\n'+'='*90)
print('VV 汇总（跨全部 219 帧）')
allbp=[]
print('TC-07 守恒专项（蓝%首→尾）:', f"{results['tc07']['b0']:.1f}→{results['tc07']['b1']:.1f} 漂移±{results['tc07']['bdrift']:.2f}%")
