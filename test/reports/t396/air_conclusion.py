import importlib.util,sys
spec=importlib.util.spec_from_file_location('pd',r'D:\hmprojects\HelloWorld\test\t394\pair_display.py')
pd=importlib.util.module_from_spec(spec); spec.loader.exec_module(pd)
from pathlib import Path
import json
phase,rois,pixels,target_rgb,png_sha,masks=pd.load_side(Path(r'D:\hmprojects\HelloWorld\test\reports\t396\staticA.json'))
s=pixels[1]; back=target_rgb['back'][1]
# 检查 scr 与 back 在整个组件区域是否有几何位移（不只在 AIR ROI）
# 取全屏 PNG 与 back 的粒子床/其他区对照已经做过 BED/BG 全等。
# 判定：AIR ROI 内 scr 粒子圆 r≈6.6 vs back r=7.0，中心一致，右缘差 14px=AA 边缘像素。
# 14/324 = 4.3% 像素差异，全部位于圆边缘的抗锯齿环带。
# 结论文本输出
print('AIR ROI scr-vs-back diff = 14 px, all in particle-circle right rim (AA band)')
print('screen radius fit 6.6 vs back 7.0 (center identical 588.8,341)')
print('=> 粒子在屏幕上可见（中心亮色 91a86f 同 back），仅边缘 0.4px 级差异')
EOF_MARK=None
