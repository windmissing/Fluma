# T987 · 目标9配对计量与目标13隔离SOURCE集显实测（effector）

2026-11-25 · 开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924，Windows x64）。GPU 驻留、实际 GPU 串行，无 Canvas 回退。

工件全核：T986 包 45/45 件 SHA 与 package.json 一致（PE `25d28ab9`）；T976 PE `873f9c98` 与 roles `721e5c09` 与 developer-T976 文档逐字一致；G9 输入沿 T923 已接受 FIXTURE128（`3580ccb1`）。

## ① 目标13 · T986 隔离 SOURCE 三臂——全过（含关键新证据）

按 developer-T986 命令三个独立进程各跑一次，全部走到 `T986_SOURCE_ONLY_CAPTURED`：

| 臂 | 路由 | 16 槽录制 | grant→accept | 250ms | 冷档 SHA |
|---|---|---|---|---|---|
| source-fast | singleton no-opt（4 目标槽 flags=1） | 16/16 | 121.88 ms | ✓ 及时 | `90aa1a98` |
| source-grouped | T963 组合（5/6/8/9=OP55_58_INSTALL/OP56_59_AUDIT） | 16/16 | 136.96 ms | ✓ 及时 | `6caac633` |
| source-singletons-opt | singleton flags=0 | 16/16 | 129.05 ms | ✓ 及时 | `3fb8b13f` |

- **槽映射逐槽核对**：PLAN==RECORDED（role/flags/sha/push/set 全等）；同 `inputSHA 790991b2`、同 `configSHA 35241d9f`（与 T978 F02 同配置）；canonical/permit 按臂策略各异（预期）。
- **关键新证据——组合角色真实编译时长首次实测**：grouped 臂 `OP55_58_INSTALL` pipeline 创建 **287.3 s**、`OP56_59_AUDIT` **302.0 s** 后均 vk=0 完成。T977 的 180 s 观察窗超窗是窗口不够长，**不是挂死**——与 T977 singleton 拆分修正（85–107 s）同族、量级更大的真慢编译。其余 8 个共同角色 ~2 ms。
- **无假提交**：三臂 `originalOutcome=1`、`firstErrorOp=0`、`latchOK=True`、`acceptedRoot=null`、`typedToken=null`（隔离诊断身份保持）；OP66 双探针 10 个 pipeline 全 QUALIFIED；`T877_FILE_WRITTEN` SHA 与落盘文件逐字一致（1,792,768 B）。
- 快臂/opt 臂 4 个 singleton pipeline 均 ~2 ms（驱动磁盘缓存暖命中，与 T977 首次冷编译 85–107 s 对照成立）。

## ② 目标9 · T976 面通量候选相邻 ABBA/BAAB 配对计量

固定：同 PE、同输入、`--inventory-single-scan`、`--quantum-detail face`、`--quantum-timestamps`、`--source-prepare-retired`、`--steps 1`。

- **r1 轮两块判缺**（按 T985 合同“任一位置失去 QUANTUM→整块重做”）：ABBA 首位 A1 SOURCE 根 VK_TIMEOUT 无 QUANTUM；BAAB 首位 B3 QUANTUM vk=1 无时间戳。jsonl+manifest 保留，raw 已清。
- **r2 轮（预热 1 次不计入）两块完整**，8 试次全取到 QUANTUM 27 点（15 粗+12 face 细）、同 fence reset=4/gen=4、SOURCE 根 vk=0、绑定行 opcode25/27 模块替换正确（A→`b4e8c61a`/`3992c136`，B→`cac34472`/`d0d7adf7`）：

| 试次 | 模式 | 整根 q0→q14 ms | 六 OP25/27 段 ms |
|---|---|---|---|
| A1 | 基线 | 1266.09（位置漂移偏高） | 549.83 |
| B1 | 候选 | 1096.56 | 455.74 |
| B2 | 候选 | 1052.80 | 440.13 |
| A2 | 基线 | 1046.70 | 452.01 |
| B3 | 候选 | 1048.48 | 432.65 |
| A3 | 基线 | 1057.30 | 463.23 |
| A4 | 基线 | 1064.37 | 462.55 |
| B4 | 候选 | 1044.50 | 425.88 |

**配对方向（T985 判据）**：
- 两块同向：ABBA B−A 整根 −81.7 ms（A1 位置漂移拉大）、六段 −53.0 ms；BAAB B−A 整根 −14.3 ms、六段 −33.6 ms。
- 六段均值 B 438.6 vs A（除 A1）459.3 → **−20.7 ms**，超过同模式相邻漂移（A2↔A3↔A4 约 10 ms）→ 面通量候选的六段收益方向**两块一致成立**。
- 整根差 −14.3 ms 处于 iGPU 漂移边缘；且六段理想化减零后整根仍约 1.05 s，**产品准时仍 0/8**（250 ms 门全部超期）——与 T985 推算（收益不足以推进 250 ms）一致，250 ms 路线仍须压缩 OP21/32/33/审计大段。
- 候选等价性沿 T977/T983 已核（21,752 S128 原字对照），本次未重跑等价臂。
- 32 个 ARENA 件按 T983 先例换 SHA 登记（`arena-sha.json`），其余 raw 原件全保留。

## 结论

- 目标13：隔离 SOURCE 三臂全过，250 ms 内及时、无假提交、16 槽身份齐全；**组合角色 287/302 s 真慢编译实测**（非挂死）为 T977 遗留问题的最终定性证据。
- 目标9：两完整块同向，候选六段收益 −20.7 ms（超漂移）方向成立；整根仍晚，产品准时 0/8，250 ms 推进仍需大段归因。单次/异向不确定已消除，转为“方向成立但不足以产品化”。
- 后续：SOURCE→QUANTUM 实际顺序接入与业务 promotion 按 T985 合同独立另行采证；250 ms 主线沿 T973 大段拆解。

报告：`test/reports/t987/report.md` + `summary.json` + `runs/`（g13-source 三臂 jsonl+冷档、g9-paired r1 判缺日志、g9-paired-r2 有效块全原件）。
