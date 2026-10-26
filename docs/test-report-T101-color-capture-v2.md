# T101：色彩采集精确续证 v2（同目标生产对照 + 连续量逐级）

日期：2026-10-26。角色：tester。任务卡：T101。合同：`docs/architecture-T93-basebyte-evidence-routing-v1.md` §3–5 与 `docs/architecture-T87-color-matrix-routing-v1.md` §4（版本 `T88_COLOR_CAPTURE_V2`；保留 53a88be/T80、f683edc/T88 原始 raw，不覆盖不重跑整套）。执行器：`test/p013_native/t101_color_capture_v2.cpp` + `build_t101.cmd`。证据：`test/reports/t101/remaining-color-capture-v2.json`（canonical）+ t101-manifest.json + production/debug 全图 raw + 五阶段 float raw + environment.txt。

## 1. 同目标生产对照（T93 §5 首项）

**同一 context、同一 SRGB8_ALPHA8 pbuffer 目标、同一 uniform 上传**：
- 生产 `MaterialPass`（真实类）先绘：**(88,78)=[238,181,85]**；
- debug MRT 变体后绘：**(88,78)=[238,181,85]，stage baseB=173 grainB=102 strataB=127 k=1**；
- 全图 160×288 逐字节 diff：**diffBytes=0**。

三个直接结论：
1. **插桩零影响**——debug 变体与生产逐字节一致，baseByte=173 证据获得 T93 要求的"同目标生产等价"；
2. **无 uniform 错误**——T93 §1 指出的"238/181/85 对 247/219/157 不能先归咎 uniform"就此关闭：差值来自**目标域不同**（见 §2），不是上传错误；
3. 生产与 debug 两个 shader hash、fixture hash（grain/strata 与 T55/T88 相同）全部入 manifest。

## 2. 连续量逐级采样（RGBA32F float target，实际支持且 complete）

(88,78) 处五阶段（每阶段独立 pass，raw float 全图归档）：

| stage | 值 | 解读 |
|---|---|---|
| gain | 1.14999998 | Float32 插值（T77 的 1.149999976158142 的 binary32）✓ |
| Qbase | (15.31, 3.05, 0.718) | preparedQ(baseByte/255)，k=1 L 带 baseByte=(248,225,173) ✓ |
| Rpre | (14.15, 2.44, 0.504) | preparedQ(base)+dc+db（grain/strata/c3 增量已加入）✓ |
| Lmat | 1 | 光照关 ✓ |
| ToneMap | (0.934, 0.709, 0.335) | r/(1+r) 线性 ✓ |

**最终字节闭合**：`round(255×ToneMap) = (238, 181, 85)` = 观测最终色——本 host 该 SRGB pbuffer 读回是**线性码**（T80 H1：`EGL_KHR_gl_colorspace` 缺失，SRGB 请求静默降级），生产数学链 gain→Q→Rpre→Lmat→ToneMap→最终字节**全程对账一致**。T55 归档的 157 是实际编码的 sRGB-FBO 路径——两者在固定目标域后完全自洽。

## 3. T87 §4 逐项到件

- T2 实际创建尝试 + `eglQuerySurface` 已执行（queryOK 落盘）；扩展缺失 → 请求降级如实记录，不改写 T80 SKIP；
- GL 扩展从当前 `glGetString(GL_EXTENSIONS)` 取，write-control=NOT_SUPPORTED，无凭空 enable；
- 生产/debug/stage 三个 shader 源 hash + fixture hash + 全图/逐阶段 raw + 自动 manifest（无人工转录）；
- default GL_BACK 域证据不重复（T80 T1 已交）；四 target 分组不重复（T80 T1/T3/T4 有效观测保留）；
- 连续量：**已采**（gain/Qbase/Rpre/Lmat/ToneMap）——T88 缺口关闭。

## 4. 分流与边界

- 半透明精度表仍为观测项（无新容差；dither 分支未重采，T88 partA raw 保留）；
- 真正编码的 sRGB 路径在本 host 不存在 → 设备 A/B/C 仍需；
- baseByte 表示分叉（172 vs 173）沿 T93 §2 → **T99 designer 设计可并行推进**，本卡证据已补齐其所需的生产等价与连续量链；
- 未改生产/shader/配方/阈值；未用真机；无 Canvas；结果回 architect，不整体归因。
