# T402 · 单沙屏幕配对与亚像素兼容裁定

2026-11-07，architect，P4。不使用真机。核 T396 PNG、manifest/sidecar、hold 封存与原 `test/t394/pair_display.py`。此稿修订 T391 的系统显示取证合同和 T394 判读条件。

## 1. 原件结果与原报告补正

五组manifest/sidecar、candidate/back原RGBA及hold身份均通过原loader校验。T394-final本地签包SHA与T396登记的 `f8503caf…e8c138` 相符。按 spec 的 `[51,1968][1229,2650]` 截取、翻转行原点、丢弃原RGBA中的alpha后直接比较RGB，复算如下：

| 样本/区域 | screenshot 对 back 的差异像素 | 原件范围 |
|---|---:|---|
| 静态 BED 16×16 / BG 16×16 | 0 / 0 | 两个区域的RGB确切相等；candidate/back对应原RGBA相等。 |
| 静态 AIR 18×18 | 14 | screen 有142个沙色像素，back有156个；差异在native坐标x590–595/y334–347。全ROI仅有沙色145/168/111和背景16/16/36，未出现混合边缘色。 |
| CONTROL_A / LOCK_A，各三ROI | 全为0 | 旧粒子、空的新支持区、背景均直接相等。 |
| CONTROL_B / LOCK_B，R0 / R1 / R2 | 0 / 28 / 0 | R1 screen和back各155个沙色像素，图案集合在全局x方向差1px，边缘差共28个。 |

静态AIR色块质心为screen x588.260563、back x588.756410；B的新粒子质心为screen x882.216129、back x883.216129，y相同。测试者将静态误记“三区逐位一致”和“圆心相同”，并从半径拟合推到GPU AA唯一因，证据范围过宽。原件支持颜色离散值与大体内容变化，但静态AIR及B的直接像素相等尚未通过。原pair对CONTROL和LOCK都给 `DISPLAY_NOT_JUDGABLE/B_SCREENSHOT_DOES_NOT_MATCH_EITHER_BACK`，保留该码。

五个manifest只有中性光学一臂，config的grain/surface/core/rim gains均为0。床/粒子色码和中性区域有窄证；纹理与非零gains窗口结果仍待取证。截图为RGB，不能由其验证屏幕alpha。native alpha独立从RGBA读取。

## 2. 先证明坐标采样，再谈兼容

原工具只要求layout整数bounds的宽高等于native读回W/H，即判 `MAPPING_REQUIRES_EXACT_PIXELS` 通过。但同宽高不证明screen-to-buffer逐像素映射，尤其此包的 `boxWidth=1177.5999755859375`、native width=1178、scale=3.5。layout本身未归档完整的浮点全局位置、纹理裁剪或composition transform。

从五张原始1280×2832 PNG的**非粒子扫描行**独立读到同样边界。例如全局y1976和2100，x0–50为页面白，x51–55为左框，x56–1221为背景，x1222–1227为右框，x1228起为页面白。实际非白连续覆盖是半开 `[51,1228)`，宽1177；纵向 `[1968,2650)` 覆盖682。该测量不使用受检AIR或新粒子的形状来拟合。

在此基础上做历史解释复算，设screen局部像素 `d`，native采样索引为

`sx=floor((d+0.5)×1178/1177)`，`sy=d_y`。

行原点仍按底左到截图顶左转换。在固定原ROI内，只有native反查索引仍落在已归档back tile的像素可判读。静态样本818/818、四个动态样本各1160/1160，共 **5458/5458 RGB相同**。另有178个像素的native反查列落到原tile外，保留未测。这个由边框覆盖宽度推导的单一全局模型，同时解释左粒子直接相等、中间静态形状少14像素和右粒子偏1px，比逐臂平移/缩圆更具体。它尚未证明框架内部的精确采样实现，也未补回缺失列，不能改写旧档为完整系统显示PASS。

原loader还只验证手填 `captureWallMs` 落在hold内，时间链缺截图工具原始调用前/后时刻。几何和时段各自都应有独立来源。接下来优先补工具与几何采集，无证据要求修改单沙材质shader、圆半径或EGL present。

## 3. Developer 最小修订合同

1. **保留原逐像素模式并版本化新增校准模式**。登记native W/H、浮点box、vp2px、XComponent浮点全局位置与大小、layout整数bounds、截图尺寸/旋转/缩放、window/surface/context/geometry和采集工具身份。能直接获得可信composition矩阵与采样规则则原样归档。取不到时，在同一几何下先跑独立标定夹具，不能通过被测A/B粒子寻找平移或半径。
2. **独立标定**。沿XComponent+shader给显式标记的诊断夹具输出左/中/右和上下不对称的已知位置色块或格点，采全屏无损PNG与同次native读点，固定采集时段和token。标定点坐标预注册、至少跨全组件范围，留未参与求解的验证点；尺寸、偏移、轴方向、旋转/镜像、裁剪及nearest/其它采样模型均须可区分。矩阵与采样规则有确定来源、验证点通过才冻结校准ID，绑定全部几何/窗口/设备/截图配置。失效后重标。诊断夹具若改shader须独立版本/hash，材质评估回到原shader。单独的layout尺寸或均匀背景不够确定映射。
3. **固定ROI与完整取数**。用authority/typed profile/xy/meta和独立几何oracle在开始前选择旧粒子、新粒子、背景三个screen核心ROI，最大64×64限制沿用；根据已冻结映射预先计算所有所需native索引及带边缘的capture tile。普通20×20核心可先考虑24×24或按映射覆盖实际取值，越上限则拒绝或另预约分组。每个screen核心像素都必须有原始native样本，不得事后剔除不匹配点、padding未知列或用背景猜缺失像素。两目标预算仍≤98,304B。
4. **独立oracle**。坐标映射与颜色转换分开。对于已验证nearest模型，按声明的pixel-center和方向做整数索引，RGB与对应back码值逐项比较。candidate/back仍在各自同native ROI比较，native alpha分列。若实际采样/颜色管理需要过滤，先通过独立校准确定模型及误差包络，记录适用域；缺规则输出不可判。CPU投影mask用于确认旧/新粒子支持集及无污染背景，F32/边界不确定点在预注册算术误差域标记，不能把native实际覆盖155与理论156的差直接归AA。系统映射未知时不得用放宽半径掩盖该差。
5. **冻结与结果码**。新的校准模式用独立版本，例如 `T402_DISPLAY_PAIR_V2`，分别输出native同token交换、映射身份、截图hold身份、screen对A/B匹配、不可判理由。已校准完整配对用独立码 `DISPLAY_CALIBRATED_A_AND_B_PAIRED`，旧像素模式保持 `DISPLAY_A_AND_B_PAIRED`。B仍映射到A时报告 `DISPLAY_REPEATS_A_IN_B_HOLD`。同一组A/B全程共用一个冻结矩阵与采样规则；禁止逐ROI/逐臂搜索平移凑通过。并保存原差异坐标、样本数和映射后差异。
6. **采集时间**。截图驱动在同一单调时钟域记录调用前后与文件SHA，整个调用区间必须在hold内。若只能跨域记录，提供映射与误差区间，连误差包络也须被hold包含。保存实际工具输出，操作者手填时刻只能作来源标记。hold取消、换代或后续帧替换时拒配。常态仍零readback，诊断GL状态/失败恢复和正常封签保持。

## 4. 离线反例和真机最小回归

先交tester消费的独立标定/判读工具，反例必须能拒绝重复A、漏B、错色（包括1码的内部常量偏差）、整体偏1px、仅B偏1px、改半径、错误width/比例/半像素原点、镜像、ROI缺列、过期校准、换context/geometry、截屏越hold和SHA污染。合法的预注册1178→1177 nearest模型、1:1模型和跨点验证是正控。真实一像素渲染错误在已经冻结的正确映射下必须失败，不能通过再求一次变换。原T394旧模式反例继续有效。

tester在新工具与必要签包交付后，先独立标定，再用原shader复测中性厚床三区（含AIR）、一组非零gains、CONTROL A/B和LOCK A/B，补完整反查tile和截图时间证据。保留前后景XComponent+shader，失败显示FAULT或有完整旧token的STALE。T396旧档解释为同native颜色/内容变化与几何映射候选模型有证，系统逐像素完整配对继续缺证；T402的兼容合同已完成。
