import importlib.util
spec=importlib.util.spec_from_file_location('pd',r'D:\hmprojects\HelloWorld\test\t394\pair_display.py')
pd=importlib.util.module_from_spec(spec); spec.loader.exec_module(pd)
from pathlib import Path
import json
b,rn2,bp,bt,bsha,bm=pd.load_side(Path(r'D:\hmprojects\HelloWorld\test\reports\t396\dynB.json'))
sb=bp[1]; ba=bt['back'][1]
x0,y0,w,h=rn2[1]
d=[j for j in range(len(sb)) if sb[j]!=ba[j]]
px=sorted(set(j//3 for j in d))
# 精确对齐：拟合平移 (dx,dy) 使 back 粒子图案与屏幕重合
scr_particle=b'\x91\xa8\x6f'
def mask(img,cx,cy,r=7.0):
    return set((x,y) for y in range(y0,y0+h) for x in range(x0,x0+w)
               if img[((y-y0)*w+(x-x0))*3:((y-y0)*w+(x-x0))*3+3]==scr_particle and True)
# 用像素色集合：屏幕粒子像素集合
def pset(img):
    s=set()
    for y in range(y0,y0+h):
        for x in range(x0,x0+w):
            p=(y-y0)*w+(x-x0)
            if img[p*3:p*3+3]==scr_particle:s.add((x,y))
    return s
S=pset(sb); B=pset(ba)
print('screen particle px:',len(S),'back particle px:',len(B))
best=None
for dy in range(-3,4):
    for dx in range(-3,4):
        moved=set((x+dx,y+dy) for (x,y) in B)
        inter=len(S&moved); err=len(S|moved)-inter
        if best is None or err<best[2]: best=(dx,dy,err,inter)
print('best shift dx=%d dy=%d mismatch=%d inter=%d'%best)
