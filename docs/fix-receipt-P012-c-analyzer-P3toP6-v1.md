# P-012 C 分析脚本 P-3~P-6 修复回执 v1

- **日期**：2026-10-22；责任：tester。对应任务【P-012·C脚本修复】（滴答 6ad96615）。
- **对象**：`tools/analyze_p012_c_verify.py`（SHA256 `0fa61f08d1ac5cb7…`），修复 pipeline-rehearsal-P012-c-verify-v1.md §3 登记的 P-3~P-6 四项。
- **入库说明**：修复内容随会话并行批次 273e27e（11:18 阶段一 CLOSED 批）入库，文件内容与本地工作副本逐位一致（SHA256 校验）。

## 1. 四项修复内容

| # | 修复 | 实现 |
|---|---|---|
| **P-3** | return 帧不再静默丢弃 | main() 按 `-static-`/`-tilt-`/`-return-` 三段全部分析；输出 `coverage.segmentsPerDir`（每目录各段帧数）+ `coverage.droppedFrames`（无相位标签的文件逐个登记原因）；return 段进逐帧表（回正残留位移检查不再盲区） |
| **P-4** | 蓝带边界抗漂移 | canonical 调色板：蓝带=行蓝占比≥0.5 的**最长连续段**（非 min/max，抗锯齿过渡行不再拉伸边界）；支持 `--bands=` 或自动探测 `band-bounds.json` **冻结边界**优先（bandSource=frozen-band-bounds）；每帧登记 `bandSource`（边界来源可审计） |
| **P-6** | 输出头 provenance 块 | `provenance`: dataRoot/batchDirName/analysisTimestamp/manifest；`collect-manifest.json` 存在时读入（tier/want/seed/build SHA），缺失时输出 manifestWarning（批次无法自证 want 参数——正是 invalid 批次的教训） |
| **P-5（硬前置）** | 两臂逐位相同哨兵 | 每对 static-0 双臂 ROI 内逐像素差分；`npx(\|diff\|>3)=0` → 顶级 `SUSPECT_NO_TREATMENT=true` + perPair 明细 + stderr 醒目告警 + **进程退出码 2**（采集 SOP 立即停止）；正常时 exit 0 |

## 2. 附带发现与修复：P-7（演练时未知）

**双调色板问题**：invalid-noTier 批（无 tier，canonical 蓝水/黄沙/绿沙饱和调色板）与有效批（p012tier=3，EAB 层把调色板洗成淡青水/奶油沙/淡绿沙）视觉结构完全不同——原脚本的单调色板 hue 掩码在有效批上**全军覆没**（42/42 帧"blue band not found"；本次演练前未发现是因为演练只跑过 invalid 批）。

修复：`frame_masks` 双调色板——canonical 掩码探测失败且淡调色板掩码命中时切换 tiered 分段（淡青水带=连续段；光沙=R≥244 持续 ≥10 行段；重沙=其后至最后强绿行）；分段规则按有效批归档帧逐行校准（水 950–1155 / 光沙 1174–1567 / 重沙 1568–1975）；每帧登记 `palette` 字段。

## 3. 回归验证（DoD 逐项）

| DoD 项 | 结果 |
|---|---|
| invalid 批回归：SUSPECT_NO_TREATMENT 应触发 | ✅ 3/3 对全部触发，`npx>3=0`，exit=2 |
| 有效批复现不受影响（CLI 参数化保持） | ✅ 42/42 帧 0 错误（dual-palette 后），哨兵不触发（p1 static-0 npx>3=692,195，两臂差异真实），exit=0；方向与归档一致（OFF oscStd 0.009 < ON 0.112，21 帧均值 OFF 0.044 < ON 0.103——ON 层纹振荡更多，与设计预期同向） |
| 正式采集前清单勾销 | ✅ P-1/P-2（CLI 参数化，前次已修）+P-3/P-4/P-5/P-6 全部落地；rehearsal §3 清单 7 项中 6 项闭环（P-7 聚合行随本次汇总输出实现，可选项） |
| 离线不占真机 | ✅ |

注意：有效批 oscStd 绝对值与 effector 归档值（0.073/0.516）不同——分带窗口与去趋势实现不同属预期（归档值来自 effector 自有分析管线，本脚本是独立通道）；**方向与量级比（ON>OFF 显著）一致**，不影响任何已归档结论（本脚本不产出效果结论）。

## 4. 三批次回归矩阵

| 批次 | 帧数 | 错误 | 调色板 | bandSource | 哨兵 | exit |
|---|---:|---:|---|---|---|---:|
| p012-c-verify-20261021（有效） | 42 | 0 | tiered | tiered-sustained-runs | 不触发 | 0 |
| …-invalid-noTier（无效） | 42 | 0 | canonical | frozen-band-bounds | **触发×3 对** | **2** |
| …-20261022-smoke（冒烟） | 4 | 0 | tiered | tiered-sustained-runs | 不触发 | 0 |

smoke 目录命名（`smoke-off`/`smoke-on`）也已支持。

## 5. 移交 effector：正式采集 SOP 更新

1. 批次目录放 `collect-manifest.json`（pipeline-rehearsal §4.2 字段表）——脚本 provenance 自动消费；缺失时输出 manifestWarning（不阻断，但采集后审计会看到）。
2. 采集后第一件事跑本脚本：**exit=2 = 废批，立即停止后续对采集**（等价于把人工检查 30 秒 SOP 自动化）。
3. `--bands=` 参数可选：若冻结边界文件就位则优先消费；否则自动分段（bandSource 登记来源）。
4. return 段帧命名保持 `{pair}-{mode}-return-{k}.jpeg`——现在会被分析（回正残留位移检查）。

## 6. 产物

- `tools/analyze_p012_c_verify.py`（SHA256 `0fa61f08d1ac5cb7ea79e00a882cfc316c49c6656f681e518e7e628fbc900615`）
- 三批次回归输出 `analysis-pixel-fixed-v2.json`（各自批次目录内）
