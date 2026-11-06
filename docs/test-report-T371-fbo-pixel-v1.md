# T371 单沙离线 FBO 像素补证（harness 交付 + MSVC 构建沙箱阻断 BLOCKED 如实）（离线，部分完成）

2026-11-06，tester。按 T370 §2/3 要求补 SM03/05/06/09、RGBA8 线性/sRGB 校准及正反控。

## 1. 已交付：独立 SM 像素 harness（可编译待跑）

- **`test/t371/host_sm.cpp`**（独立于 T364 host.cpp，不修改任何 T364/产品文件）：复用生产 `SingleSandPass::Draw`、同 GLSL（prepare_host.py 生成链、shader SHA `f22a5e86…` 保持）与 `TestCommittedFramebuffer()` seam。
- **覆盖臂**：
  - **SM12 校准**：独立 `GL_RGBA8` 线性 FBO（GL_LINEAR 编码断言）+ 6 点灰阶 ramp 0/128/145/168/111/255 冷读回（EOTF-free 原码 ±1 容差）；生产 sRGB candidate 走原 `TestCommittedFramebuffer()` 分列。
  - **SM03**：背景 32×32 ROI 均匀性冷读回（bg 16/16/36 常量）+ **错误 albedo 反控**（改 background values → 背景码必须改变）。
  - **SM05**：flat 床 vs 线性坡 profile 两臂 R 通道和差分（表面深度 shading 响应）+ **确定性负控**（同输入两帧 ROI 字节必须相同）。
  - **SM07/SM06**：state2 粒子床内置入 → **独立几何 oracle 掩膜**（半径/中心解析生成，非被测渲染）内 inMask/outMask(spill) 结构断言 + `D=abs(Ymix−Ybed)` 逐掩膜像素统计（mean/D≥4 比例，候选线记录不设门）。
  - **SM09**：s=1/2/3 scale 臂 lit-pixel 非降监测（栅格相位差 caveat 如实）。
  - PPM 原始输出存档（`t371_sm03_background.ppm`/`t371_sm05_flat_bed.ppm`/`t371_sm05_slope_bed.ppm`/`t371_sm07_with_particle.ppm`）。
- **`test/t371/build_sm.cmd`**：与 T364 build_host.cmd 同构（同 vcvars/同源文件列表/host_compat），输出独立 `test_sm.exe` 不覆盖 T364 test.exe。

## 2. BLOCKED：MSVC 构建环境（沙箱级，原错如实）

- 原错：`cl` 全部编译单元 `fatal error C1083: crtdbg.h: No such file or directory`——`vcvars64.bat` 内部调用 `reg.exe` **被本沙箱程序黑名单拦截**（PROGRAM BLOCKED BY SECURITY POLICY），INCLUDE/LIB 环境未建立。
- 尝试：①修正 build_sm.cmd `/Fo` 多源文件语法（D8036→目录形式）；②对照运行 **T364 原 build_host.cmd 同样失败**——排除本卡脚本缺陷，确认为环境级阻断（T364 当时在沙箱外交互终端构建）。
- **输入前置**：在沙箱外终端执行 `test/t371/build_sm.cmd`（一条命令即产出全部 SM 臂证据+PPM）。
- **接手角色**：developer（沙箱外跑一次并回传输出/PPM），或用户本地执行。

## 3. 可做部分已完成

- SM12 线性 ramp oracle 逻辑（EOTF-free 判据）已实现于 harness；SM04 离线负控可推导：常数 tile（T364 128-fill fixture）→ 纹理方差期望严格 0（与 SM04"aG=0 差严格 0"反控一致，生成器 hash 已由 T365 独立复现）。
- T361 分母核验已由 T363 交付（rawCallbackInterval p50=767.7ms 在案）——本卡不重复。

## 4. 边界

未改产品/T364 资产；新文件仅 `test/t371/`；未跑真机；正反控均入 harness；原始 RGBA/PPM/SHA 产物由 harness 运行时生成（沙箱外执行后回传归档）。
