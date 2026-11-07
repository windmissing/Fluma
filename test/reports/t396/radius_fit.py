import importlib.util,sys
spec=importlib.util.spec_from_file_location('pd',r'D:\hmprojects\HelloWorld\test\t394\pair_display.py')
pd=importlib.util.module_from_spec(spec); spec.loader.exec_module(pd)
from pathlib import Path
import json
phase,rois,pixels,target_rgb,png_sha,masks=pd.load_side(Path(r'D:\hmprojects\HelloWorld\test\reports\t396\staticA.json'))
s=pixels[1]; back=target_rgb['back'][1]; cand=target_rgb['candidate'][1]
# scr 与 back 唯一差异：粒子圆右缘 14px（scr=BG 色 101024, back=粒子色 91a86f）
# cand 与 back 相同（cand==back 全等已证）。
# 圆心 x=588.8 r=7 → 圆右缘至 595.8；back 中 595 列在圆内(91a86f)，屏幕在 595 列已是 BG
# scr==back 的边界在 x=594/595 之间 → 屏幕上粒子圆半径 ≈5.2px? 中心不变。
# 验证：对 scr 重建圆（中心 588.8,341.0），扫半径 r 使 |scr 重建-back| 最小
best=None
for r10 in range(40,80):
    r=r10/10
    pred=set((x,y) for y in range(332,350) for x in range(579,597) if (x+.5-588.8)**2+(y+.5-341.0)**2<r**2)
    err=0
    for y in range(332,350):
        for x in range(579,597):
            p=(y-332)*18+(x-579)
            isin=(x,y) in pred
            scr_is_particle = s[p*3:p*3+3]==b'\x91\xa8\x6f'
            if isin!=scr_is_particle: err+=1
    if best is None or err<best[1]: best=(r,err)
print('best radius fit scr: r=%.1f err=%d'%(best[0],best[1]))
# 同法验证 back
best=None
for r10 in range(40,80):
    r=r10/10
    pred=set((x,y) for y in range(332,350) for x in range(579,597) if (x+.5-588.8)**2+(y+.5-341.0)**2<r**2)
    err=0
    for y in range(332,350):
        for x in range(579,597):
            p=(y-332)*18+(x-579)
            isin=(x,y) in pred
            back_is_particle = back[p*3:p*3+3]==b'\x91\xa8\x6f'
            if isin!=back_is_particle: err+=1
    if best is None or err<best[1]: best=(r,err)
print('best radius fit back: r=%.1f err=%d'%(best[0],best[1]))
EOF_MARKER_NOT_USED=0
