import importlib.util,sys
spec=importlib.util.spec_from_file_location('pd',r'D:\hmprojects\HelloWorld\test\t394\pair_display.py')
pd=importlib.util.module_from_spec(spec); spec.loader.exec_module(pd)
from pathlib import Path
import json
phase,rois,pixels,target_rgb,png_sha,masks=pd.load_side(Path(r'D:\hmprojects\HelloWorld\test\reports\t396\staticA.json'))
s=pixels[1]; back=target_rgb['back'][1]; cand=target_rgb['candidate'][1]
diff=[i for i in range(len(s)) if s[i]!=back[i]]
px=sorted(set(i//3 for i in diff))
coords=sorted(set((p%18+579,p//18+332) for p in px))
print('diff coords:',coords)
m=json.load(open(r'D:\hmprojects\HelloWorld\test\reports\t396\t394-1-1-1628.json',encoding='utf-8'))['probe']
radius=m['configValues'][6]*m['scale']
cx,cy=588.8,341.0
in_circle=set((x,y) for y in range(332,350) for x in range(579,597) if (x+.5-cx)**2+(y+.5-cy)**2<radius**2)
print('radius',radius,'diff∩circle:',len(set(coords)&in_circle),'diff-only:',len(set(coords)-in_circle))
for c in coords:
    p=(c[1]-332)*18+(c[0]-579)
    print(c,'scr',s[p*3:p*3+3].hex(),'back',back[p*3:p*3+3].hex(),'in_circle',c in in_circle)
