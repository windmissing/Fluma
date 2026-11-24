# T954 · 目标9裁槽、13流场与集成Demo出生观察（effector）

2026-11-24 · 开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924）。GPU 驻留、shader 渲染，无 Canvas 回退，250ms typed 门原值。

工件：`.codex/local-test/t946-resume-package`（manifest `482d5f4f…` 122 件全核，主 PE `d9e6f89c`、DM2 控制 `67c6d7b1`、DM2 间接 `dd55cb8c`）；执行中发现 T946 v2 PE 不支持 `--g13-cycle-scope/seal-stages`（usage 拒），该功能实现在后交付的 T955 PE（`eb0e841e`，SHA 与 developer-T955-v1.md 收据一致）——目标13 的 seal/cycle 证据改用 T955 同源实现采集并如实标注。

## ① 目标 9 · REPLAY 裁槽（control vs replay-indirect）

同 seed（T923 steps1-r2 FIXTURE128）同窗 `--steps 1 --source-prepare-retired --quantum-timestamps --raw-envelope`：

| 判据 | control r3 | replay-indirect r1 |
|---|---|---|
| SOURCE 根 | accepted vk=0（14 派发） | accepted vk=0（14 派发） |
| QUANTUM 根 | vk=2 GRANT_DEADLINE，2404 派发，tailRead=false | 同左（encodedDispatches 2404） |
| selectedJ（迟到完成） | 1 | 1 |
| WORK control[16]（byte 1166976+64，T953 式） | **2404** | **2404** |
| 零工作组命令 | —（直接 2404 dispatch） | **372**（J=1 合同值 ✓） |
| replayIndirectArgsValid | — | true（args[96]==J、m=0 激活、31 组零、结构词全 1） |
| extraIndirectBarriers | — | 1 |
| 同 fence | Q root nonce/fence/gen == GPU 时标行（15 点全 available vk=0，queryResetSerial 4） | 同左 |
| BASE/ACTUAL word16 | 1→2（lastNonce 演化，非逐槽计数） | 同左 |

**原字比对**：`EVENT.bin`、`PATH.bin`、`FIXTURE128.bin` 逐字节相同；其余件差异词全部落在结构化身份/时钟/密封区（COMPLETION 10-15+88-95 seal、INPUT 4-9+16-21、REPLAY/SELECTED 4-9+260-277+299-300 迟到完成头+82458-61 seal、WORK 40 词 @1164944+ 间接表头）——**业务字段（REPLAY/SELECTED 主体、拒绝前缀）逐字相等**。

**J 变体**：J2/J4/J0 在本机该 seed 不可达——J 由真实迟到完成时序选出，两次实测均 J=1（迟到完成总在窗内到达）；steps2 臂 SOURCE 250ms 边缘拒（摆动，非 J 证据）。J0 语义（全拒/全排程保留）依 T953 schedule(b) 判据登记为当日不可复现限制。

**结论：过（诊断级）**——同 fence latch、control[16]=2404 保序两臂一致、零组 372 精确匹配 J=1 合同、参数表有效、业务原字等价。

## ② 目标 13 · 三闭包资格 + F02/F03

**资格（resume PE 四角色，角色名校验已修——T949 usage 拒消失）**：
- `OP48_VISCOSITY`（812db31f）/ `OP49_PRESSURE`（ca603a59）：CHILD_EXIT vk=0 **资格通过**；
- `OP_48`（f01959cc）/ `OP48_REMAINDER`（64101778）：CALL_UNRETURNED（igc64 编译挂死持续，dump 在档）。

**F02 seal-v1 ABBA（T955 PE）**：A 臂（+`--g13-seal-v1`）`OP_00_9_11_SEAL_V1` 管线×4 EXIT vk=0；B 臂（+`--g13-seal-stages`）三阶段 `OP11_T955_PREPARE/SHA/PUBLISH` 各×4 EXIT vk=0（819 三派发拆分 live 实建）；replay 臂同构。三臂均 GRANT_DEADLINE（迟到根）+ `T955_ORIGINAL_RETIRE` 5s 确认 vk=0（迟到完成实证）+ acceptedRoot=null。

**逐 cycle 与 815-819（61 GPU 点，绑同 fence/nonce/gen 1）**：ACTUAL/REPLAY spans、`ACTUAL/REPLAY_J1_MICRO0_PRESSURE_CYCLE0_BEGIN/END`（cycle0 实测 94.97/60.28ms）、TRIAL J2/J4 边、`FINAL_AUDIT`（815/816，0.572ms）、`TIME_PREPARE`、`PACK_VIEW`（817）、`PACK_AUDIT`（818）、`SEAL`（819）。ACTUAL 压力全段 701.97ms、REPLAY 587.79ms——迟到根量级与 T901/T949 一致。冷档 `.quantum` 1,795,120B×4 臂封存（t955PrivateLateRaw=true）。

**F03 实际黏性**：VISCOSITY/PRESSURE×4 EXIT、GRANT_DEADLINE 迟到拒、ORIGINAL_RETIRE vk=0——**未接受 → 按卡面规则不判场/不交 GridView**。

**结论：部分**——资格 2/4（两 igc 挂死）、F02 seal-v1 ABBA+staged seal+cycle 815-819+同 fence 全采（迟到根私有诊断），F03 无接受。

## ③ 集成 Demo · observe-birth 三场景 + 回归

`--m1-observe-birth 8`（resume PE + m1/35 模块）：

| 场景 | 帧数 | 出生像素 | 自由点轨迹（质心 y） |
|---|---|---|---|
| 中心 (312,215) | 4 | ROI0 质心 (312.0,215.0)==指针中心 ✓ | 215.0→215.3→215.7→216.0（屏幕向下） |
| 高位 (312,120) | 8 | (312.0,120.0)==指针 ✓ | 120.0→120.3→120.7→121.0→120.7→121.3→122.0→122.5（加速：Δ0.3→0.7） |
| +20°（angle256 5120） | 6 | 指针处 ✓ | 粒子窗 y 207→208→209→210→212→214（倾斜下加速下落，方向=屏幕下） |
| −20°（angle256 −5120） | 2 | 指针处 ✓ | 207→208（早停，同向） |

- 三 ROI 合同（指针/指针下方/跟踪粒子）逐帧输出；`mappedSand` Q20 高度字递减（粒子下落账目一致）；全帧 `pacedPresent=true`。
- **箭头方向**：所有场景 y 单调增——重力投影向屏幕下 ✓；±20° 对称。
- **回归（T933 用户入口 PE 4a40a141）**：4s 持按 97 量子；60s 持按 1,602 量子（26.7 Q/s）；末段 1,759；**守恒闭合** admitted 59,122,246 == stock+active(0)。无退化，用户入口保持 T933 启动器（resume PE 的 m1/35 为并行库存审计档，吞吐低系设计差异非退化）。
- 负证据：首跑 center NO_MATCHING_ROOT（Q TAIL_REJECTED 时序）→ 重试过；观察帧数据 4 words/px（BGR）解码已验。

## 结论表

| 子项 | 判定 |
|---|---|
| 目标 9 裁槽 | **过（诊断）**：J=1 零组 372、control[16] 2404、同 fence、业务原字等价；J2/J4/J0 本机不可达如实登记 |
| 目标 13 资格 | 2/4 过（VISCOSITY/PRESSURE）；OP_48/REMAINDER igc 挂死持续 |
| 目标 13 F02 ABBA | **过（迟到根诊断）**：seal-v1/staged/cycle 815-819/同 fence 全采；F03 未接受不判场 |
| Demo observe-birth | **过**：三场景出生像素==指针、≥3 自由点屏幕下方向；回归过；用户入口不更新（T933 已达标） |

证据根 `test/reports/t954/runs/`（goal9 raw×4+jsonl、qualify-g13 收据+dmp、goal13 T955 四臂+quantum 归档、demo-birth 三场景+回归）。
