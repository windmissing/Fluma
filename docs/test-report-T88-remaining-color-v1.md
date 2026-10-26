# T88：材质剩余色彩补证 v1（fractional-alpha 合成 + baseByte 精度分叉直证）

日期：2026-10-26。角色：tester。任务卡：T88。依据：`docs/analysis-T77-material-color-v1.md` §6.1 phase2 / §6.2 与 `docs/architecture-P013-color-routing-revision-v1.md`（T84）§3/§4。执行器：`test/p013_native/t88_remaining_color_test.cpp`（Part A）、`t88_stage_probe.cpp`（Part B，debug MRT 变体）+ `build_t88.cmd` / `build_t88_stage.cmd`。canonical 证据：`test/reports/t88/remaining-color-evidence.json` + partA/stage raw JSON + environment。不重跑 T80 phase1 作唯一证据；T55/T80 历史零覆写；未用真机。

## 1. Part A：fractional-alpha 线性预乘 over（§6.1 phase2）

方法：同一 FBO，back 不透明层（线性 0.25/0.5/0.75，UNORM8 量化后 64/128/191），front 预乘 over（`blendFuncSeparate(ONE, ONE_MINUS_SRC_ALPHA,…)`），α∈{0, 0.25, 1/3, 0.5, 0.75, 1}；另设纯 B / 纯 F 参照区。

- **RGBA8 FBO（LINEAR attachment）**：over 读回与 `round(255·(a·F+(1−a)·backQ))`（backQ 为量化后 dst）在 6 个 α 中 **5 个逐字节精确**；唯一 a=0.75 的 G 通道恰落在半码边界（108.500003）读 108（half-up 为 109）——与 T80 的 187/188 同类实现取整观察，记录不冻结。
- **SRGB8_ALPHA8 FBO**：合成发生在 LINEAR、写时**编码恰好一次**：读回 = 255·OETF(线性 over)，全部 6 个 α 在 ±1 内。无双重编码、无 73 码型异常。
- 执行器模型修正两次（非判据变更）：scissor 重启用遗漏；over 区域需与 back 重叠（首版 dst alpha=0 导致读回纯 a·F）。第一版期望模型用未量化 double dst 出现两通道 ±1，改为量化 dst 后精确——**是执行器模型修正，不是 GPU 异常**。

**结论**：半透明线性预乘 over 数学与混合因子在本宿主正确；§6.1 phase2 无异常项。

## 2. Part B：(88,78) GPU baseByte 直证（§6.2 首个决定性中间量）

方法：debug MRT 变体 fragment（数学与生产逐行一致，`layout(location=1) out uvec4` 捕获 `baseByte.b / grainByte.b / strataByte.b / k`，独立 hash `…`已归档 JSON），同一真实 tdata/grain/strata 上传管线，RGBA8 + **RGBA32UI** 双 attachment（`GL_EXT_color_buffer_float` 在本宿主存在，MRT complete）。整数量取整数精确；连续量（gain/Rpre/Lmat/ToneMap）本轮**未采**（float MRT 管线未铺完，按合同记缺口不倒推）。

**(88,78) 结果：`baseB=173`（整数精确）**，k=1（L 带，与 T77 一致）、grainB=102、strataB=127，5×5 邻域已归档。

与 T77 §5 对照：

| 项 | T77 预测 | T88 直证 |
|---|---|---|
| CPU double：150×gain → floor(+0.5) | 172（172.4999964） | —（参考侧） |
| GPU binary32 | 候选 173 | **173** ✓ |
| base=173 参考蓝 | 156.5342 → 量化 157 | T55 归档 GPU 蓝 = 157 ✓✓ |

**FBO 1.956 码残差在该点定性完成**：冻结 F64 参考取 baseByte=172，真实 GPU float 管线取 173——**相同输入下的精度/表示/取整分支分叉**。输入/索引/资产错误已排除（k 带与 texel 交叉一致）。

## 3. 按 T84 §4 分流

命中**"GPU baseByte/texel 与冻结参考不同"**行：同输入、精度/表示/离散取整分叉 → **转 designer 制定局部表示 revision 与证明**（不能仅上传列中心 baseByte，fH 逐 fragment 变化须定义整像素契约）。非工程输入偏差（输入已核），**不可用 epsilon**，不改 gain/palette/配方，不把 F32 影子升主参考。

## 4. 边界与缺口

- debug 绘制的最终色（238,181,85）与 T55 归档（247,219,157）不同——debug 路径 uniform 上传独立、SRGB pbuffer MRT；**阶段整数为证据，最终色等价不作主张**（已列缺口）。
- 连续 GPU 中间量未采；设备 A/B/C 仍需；无 swap/呈现证明。
- 未改生产/配方/阈值；debug shader 独立 hash；T55/T80 历史保留；未用真机。
