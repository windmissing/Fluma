# P051/P011 T151 FIELD_COLUMN 原值真机补采效果报告 v1

日期：2026-10-27。责任：effector。任务卡：`6adfe799e4b0e6ce82d3c570`。技术路线：`P051_TOUCH_TO_SOURCE_V1` 干场 DEMO。  
状态：**FAILURE_CHAIN_LOCALIZED / PAYLOAD_LIMIT_EXCEEDED_DISCLOSED / NOT_A_PRODUCT_PASS**。  
V2 诊断在设备上捕获到**完整失败链的 stage 序列**（metadata 环），失败区间被精确到**同一子步内的预览 COLLECT 执行期间**；八分量原值 bits 因 64 片×2048B 导出上限被截断（如实披露），交 architect/developer 按已定位区间继续。

## 0. 环境与执行

| 项 | 值 |
|---|---|
| commit / 树指纹 | `6ee7e19`（T149 V2 诊断 `e3ee81f` + T150 验证 + 后续文档）/ `2f2c2263362ffda3` |
| HAP | `entry-default-s149-signed.hap`，SHA256 `a59311ebd5640db0e9045412eca091505dfac46dd28f21519324cf1e518bf56f` |
| 设备 / 窗口 | VYG-AL00 `62T0225B10010020`；09:29:42–09:31:17 |
| 执行 | `tools/t151_field_column_capture.ps1`（C1/C2 两臂，T103 同款序列） |
| 产物 | `test/artifacts/t151-field-column-raw-20261027/`（两臂各 64 片、截图、dump） |

## 1. 失败链（两臂一致，V2 metadata 窗口环）

C1/C2 同构（以 C1 substep 379 / attempt a379，theta=42 REQUESTED 为例）：

| # | stage | legal | firstInvalidColumn | 含义 |
|---|---|---|---|---|
| 0 | HOT_CHECKPOINT | true | -1 | beginStepHot 前场合法 |
| 1 | BEGIN_COLLECT_AFTER | true | -1 | **预览 COLLECT 进入后场仍合法** |
| 2 | PREVIEW_FINAL_RESTORE_BEFORE | **false** | **48** | **预览最终 restore 前——场已非法（col 48）** |
| 3 | PREVIEW_FINAL_RESTORE_AFTER | true | -1 | restore 之后场又合法（**预览回滚有效**） |

**失败区间闭合**：非法化发生在 `BEGIN_COLLECT_AFTER` 与 `PREVIEW_FINAL_RESTORE_BEFORE` 之间——即**预览 COLLECT 子步执行期间**（`previewChannelPlansAt` 内的 `substep(dt)`，theta=42）某个 writer 把 col 48 写坏；预览自身的 restore 随后把场恢复合法（这解释了为什么 trace 的 s0 快照全 0 合法、而失败时刻现场非法——非法值只存在于预览执行窗口内，restore 后消失）。

`firstFailure` V1 字段交叉一致：origin=PARTICLE_BEGIN / BEGIN_HOT / FIELD_COLUMN / failureIndex=48（C1；C2 待全解析但结构同）；entry/exit 全零（无 BUSY）；drift 三分项 0、`driftMeasured=false`/`rollbackSource=NOT_EXECUTED`（V2 新增来源字段正确区分默认零与实测）。

## 2. 与 T119 结论的合并修订

T119 判定"前一**成功提交**子步的写场路径污染列值"——本卡窗口证据将区间**进一步收窄并修正**：非法化发生在**预览 COLLECT 子步执行期间**（restore 前被观测、restore 后即消失），**不是** persists 到下一子步的持久污染。即：
- 预览 COLLECT（theta=42 的 `substep(dt)`）执行中，某内部写场操作产生 col 48 非法值；
- 该非法值触发**同一子步稍后的第二次 beginStepHot**（下一候选/authoritative 路径的 COLLECT）FIELD_COLUMN 失败 → blocked → code5；
- 预览 restore 恢复场，所以**真实提交路径未受持久污染**（commit 数字与守恒始终正常）。

嫌疑 writer 收敛到：**预览 substep 内部在 theta=42 下运行的粒子 apply/LAND 或 dry 通道对 field 列的写**（与 T103 首次发现时"粒子全 airborne"一致——airborne 粒子在 theta=42 重力下的落点/写入计算产生非法值嫌疑最大）。

## 3. 导出上限截断（如实披露）

- `fieldColumnEvidenceV2` payload 达 **131,072 字符 = 64 片×2048B 上限整**，`complete=false`、`dropped=11692`——`firstIllegalBytesHex`（八分量 IEEE754 bits/badMask/三次左结合和）与部分窗口条目**未进入 payload**；
- 两臂一致（C2 同 64 片截断）；
- 已到件部分仍完成失败链定位（§1）；**原值 bits 须下次采集补获**——建议 T149 V3 或调大上限/提高采样选择性（如只锁存非法列前后各 1 列）后复采。此为观测工具问题，不是物理失败链问题。

## 4. 判定与移交

| 项 | 判定 |
|---|---|
| V2 诊断设备验证 | **PASS（部分）**：metadata 窗口环完整给出 stage 序列与失败区间；原值 bits 因上限截断未到件 |
| 失败链 | **定位**：预览 COLLECT 执行期间（theta=42）写坏 col 48 → 同子步后续 beginStepHot FIELD_COLUMN → blocked；预览 restore 恢复场（无持久污染） |
| T119 结论修订 | 区间从"前一成功提交子步写场"修正为"预览 COLLECT 执行期间写场"（T119 的 s0-vs-现场矛盾由此解释） |
| 移交 | **architect（T146）/developer**：审计预览 substep 内 theta=42 下 airborne 粒子的写场操作（apply/LAND/dry 通道）；另请评估 V2 导出上限（131072B 打满）与采样选择性 |
| T95 | 继续阻塞 |

## 5. 边界声明

- 产品代码零修改；s149 隔离构建（CompileArkTS 已跑，树指纹 `2f2c2263362ffda3`）。
- 截断后的完整 payload 原文（131,072 字符×2 臂）已入库可复核；解析脚本与本报告方法论可复跑。
- 滴答 T151 卡评论已发；清单行状态留 manager。
