import importlib.util
spec=importlib.util.spec_from_file_location('pd',r'D:\hmprojects\HelloWorld\test\t394\pair_display.py')
pd=importlib.util.module_from_spec(spec); spec.loader.exec_module(pd)
from pathlib import Path
import json
b,rn2,bp,bt,bsha,bm=pd.load_side(Path(r'D:\hmprojects\HelloWorld\test\reports\t396\dynB.json'))
sb=bp[1]; ba=bt['back'][1]
d=[j for j in range(len(sb)) if sb[j]!=ba[j]]
px=sorted(set(j//3 for j in d))
x0,y0,w,h=rn2[1]
coords=sorted(set((x0+p%w,y0+p//w) for p in px))
cx,cy=883.2,340.98
# 拟合屏幕粒子半径（同 A 侧方法）
best=None
scr_particle=b'\x91\xa8\x6f'
for r10 in range(50,90):
    r=r10/10
    err=0
    for y in range(y0,y0+h):
        for x in range(x0,x0+w):
            p=(y-y0)*w+(x-x0)
            isin=(x+.5-cx)**2+(y+.5-cy)**2<r**2
            sp = sb[p*3:p*3+3]==scr_particle
            if isin!=sp: err+=1
    if best is None or err<best[1]: best=(r,err)
print('B screen radius fit: r=%.1f err=%d'%best)
best2=None
for r10 in range(50,90):
    r=r10/10
    err=0
    for y in range(y0,y0+h):
        for x in range(x0,x0+w):
            p=(y-y0)*w+(x-x0)
            isin=(x+.5-cx)**2+(y+.5-cy)**2<r**2
            bp_ = ba[p*3:p*3+3]==scr_particle
            if isin!=bp_: err+=1
    if best2 is None or err<best2[1]: best2=(r,err)
print('B back radius fit: r=%.1f err=%d'%best2)
