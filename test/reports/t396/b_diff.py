import importlib.util
spec=importlib.util.spec_from_file_location('pd',r'D:\hmprojects\HelloWorld\test\t394\pair_display.py')
pd=importlib.util.module_from_spec(spec); spec.loader.exec_module(pd)
from pathlib import Path
b,rn2,bp,bt,bsha,bm=pd.load_side(Path(r'D:\hmprojects\HelloWorld\test\reports\t396\dynB.json'))
sb=bp[1]; ba=bt['back'][1]
d=[j for j in range(len(sb)) if sb[j]!=ba[j]]
px=sorted(set(j//3 for j in d))
x0,y0,w,h=rn2[1]
coords=sorted(set((x0+p%w,y0+p//w) for p in px))
cx,cy=883.2,340.98
import math
radius=7.0*0  # placeholder
m=b  # phase
sample=m['sample']
# 从 projectionXY/meta 重算 id2 半径: probe configValues[6]*scale
import json
probe=json.load(open(r'D:\hmprojects\HelloWorld\test\reports\t396\t394-3-1-1630.json',encoding='utf-8'))['probe']
radius=probe['configValues'][6]*probe['scale']
print('radius=',radius)
in_circle=set((x,y) for y in range(y0,y0+h) for x in range(x0,x0+w) if (x+.5-cx)**2+(y+.5-cy)**2<radius**2)
print('diff px:',len(px),'circle px:',len(in_circle))
print('diff==circle?',set(coords)==in_circle)
print('diff∩circle',len(set(coords)&in_circle))
# 屏幕上圆心像素
for c in [(883,341),(880,341),(887,341),(883,335),(883,347)]:
    p=(c[1]-y0)*w+(c[0]-x0)
    print(c,'scr',sb[p*3:p*3+3].hex(),'back',ba[p*3:p*3+3].hex())
