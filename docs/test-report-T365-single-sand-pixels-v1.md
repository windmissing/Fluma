# T365 双态单沙独立像素验证（SM01-12 离线目标独立登记；读回窗口 NOT_RUN 如实）（离线，可做部分完成）

2026-11-06，tester 离线。验证对象：T364 交付 `22d1f3a`（签包 `33d39593…cf56b3`，shaderSourceSha `f22a5e86…`）。依据：T362 SM01-12（docs/algorithm-T356-single-sand-dual-material-v1.md §129-141）+ developer-T364 v1。

## 1. 独立复跑（本卡产出，非复用开发断言输出）

- `test/t364/units.cjs` 独立重跑 → `test/reports/t365/t364-units-rerun.json`：**PASS（exit 0）**。
- 逐项比对归档 `ets.json`：**generatorHash 逐位一致**（`fb09fe15…889cb3`）、**R8 tile 两枚逐位一致**（feature 2 = `3b0a40d6…c37c5e0a3b` **与 developer-T364 文档声明值精确相符**；feature 3 = `ba3f3d1b…`）。
- T216 既有回归（exact70/material/publication）同步复跑 PASS（无回归基线确认）。

## 2. SM01-12 逐条登记（离线目标实际覆盖）

| SM | 条目 | 离线判定 | 依据 |
|---|---|---|---|
| SM01 同色 | 中性色期望 (145,168,111) | **PASS（host 精确命中）** | host.txt：sRGB FBO 中性色 **145/168/111/255** 实际读回两帧一致 |
| SM02 色度与域 | 隔离中间线性、finite 无裁切 | **PASS（部分）** | raw F64 parity + 显示 F32 parity（units 断言）；逐像素域扫描未做（NOT_RUN） |
| SM03 背景可见 | CR P05/P50 统计 | **NOT_RUN** | 需窗口/读回逐像素统计，host 仅单点读回（背景 1/1/4 在案） |
| SM04 床纹理 | 纹理 ON-OFF、RMS 0.5-8 码 | **PASS（生成器侧）** | R8 tile 生成器 hash 独立复现一致；RMS 像素统计 NOT_RUN |
| SM05 表面 | 深度剖面 faceDelta | **NOT_RUN** | 需逐像素剖面读回 |
| SM06 重叠 | BED_ONLY 消融 D 统计 | **NOT_RUN** | 需 BED_ONLY/BED+PARTICLE 双帧消融（T364 未出该消融目标） |
| SM07 几何/alpha | coverage/mask 结构清单 | **PASS（封签域）** | units：state0/1/2、CONTACT_WAIT、单 H 库存封签拒绝路径全过；像素级 coverage NOT_RUN |
| SM08 密实与数量 | S_D 驱动覆盖、账本计数 | **PASS（封签域）** | units packet.count 断言（注入后 >0）；像素密度 NOT_RUN |
| SM09 尺度 | s=1/2/3 相位逐像素 | **NOT_RUN** | 需逐像素对参考 |
| SM10 稳定 | 三次重绘 hash 相同 | **部分 PASS** | generator hash 冷跑确定性一致（两次独立执行同 tile）；三次重绘帧 hash NOT_RUN |
| SM11 同 token/故障 | 身份/mismatch/swap 失败 | **PASS（host+units）** | host：注入 swap 失败 STALE、context 重传；units：fieldEpoch mismatch→SINGLE_PUBLICATION_MISMATCH、WET/L/phase 拒绝→UNSUPPORTED_MATERIAL_SET、零 commit 拒绝——全部显式 failure 字符串 |
| SM12 颜色链 | 线性中间/sRGB FBO/窗口分列 | **PASS（前两级）** | host 分列保留：sRGB FBO 145/168/111 与 pbuffer 线性 72/100/40 **分开记录**；最终窗口 NOT_RUN |

## 3. 汇总

- **离线可证 7/12**（SM01/02 部分/04 生成器/07 封签域/08 封签域/10 部分/11/12 前两级）、**NOT_RUN 5 项**（SM03/05/06/09/逐像素域）——全部需要**设备窗口逐像素读回**（T364 运行路径无像素读回，独立读回卡待 manager 派）。
- 无损 8bit 期望 (145,168,111) 精确命中（SM01 判据核心）——中性色链最关键单点已证。
- 主观窗口未测如实登记（本卡不含）。

## 4. 证据 SHA256（登记）

host.txt `37a7df2f…`、legacy-scene.txt `51c77867…`、legacy-strip.txt `bf45484c…`、legacy-native-scene.txt `07034080…`、legacy-native-strip.txt `72e0e58c…`、ets.json `2dd35d12…`、t364-units-rerun.json（本卡新产出）。

红线遵守：未改业务/shader；仅 XComponent+shader 路径核验；窗口与主观未测如实 NOT_RUN。
