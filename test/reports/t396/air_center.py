import importlib.util,sys
spec=importlib.util.spec_from_file_location('pd',r'D:\hmprojects\HelloWorld\test\t394\pair_display.py')
pd=importlib.util.module_from_spec(spec); spec.loader.exec_module(pd)
from pathlib import Path
import json
phase,rois,pixels,target_rgb,png_sha,masks=pd.load_side(Path(r'D:\hmprojects\HelloWorld\test\reports\t396\staticA.json'))
s=pixels[1]; back=target_rgb['back'][1]
# 粒子圆心 (588.8,341.0) r=7 —— 屏幕在圆的左半 (x<=594 部分?) 差异只在圆的右缘 14px
# 检查圆心附近像素 scr 色
for (x,y) in [(589,341),(590,341),(592,341),(595,341),(596,341),(594,341),(588,341)]:
    p=(y-332)*18+(x-579)
    print((x,y),'scr',s[p*3:p*3+3].hex(),'back',back[p*3:p*3+3].hex())
# 结论假设：粒子中心在屏幕 (588.8,341)→屏上圆心亮色；而 back 在圆心也亮色；差异只在圆右缘=屏幕未画/粒子在屏幕上位置略左移
# 计算 scr 中粒子亮色(91a86f 反色? 不对 BED=91a86f 绿床) —— scr 圆心是什么？
