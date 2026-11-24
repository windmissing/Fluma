# T939 · PC G16 多盘正例与活动槽原子拒绝复测（effector）

2026-11-24。T937 完成后同一集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924）。T936 修复版 bundle `.codex/local-test/t936-g16-bundle`（manifest 逐件核过：PE `b03fd782`、注入夹具 `cfa50b74`、模块 `786bb1db`、种子 d74e/eca4 与 T928 不变）。GPU 驻留、shader 渲染，无 Canvas 回退。证据根 `test/reports/t939/runs/`。

## ① G16-A：复用 T932

`da1ed011` 已交：T930 判据 errors=[]、TOI 0.05ms 门内 350 倍余量、夹根符号确定。本卡不重复采证。

## ② G16-B 两档两速正例——受阻（物理层原子拒绝 8/8）

T936 修复生效实证：**word13 flags 断言不再触发**（T932 的 reason 2 消失）。但全部 8 组正例臂在下一层物理门拒绝：

| 组 | 结果（firstReason/Stage） | B（候选边） | C（有效边） | Ic |
|---|---|---|---|---|
| B13 static ×2 根 | **10 / 4** | 200 | 27 | 0 |
| B13 slow / fast / hold | **10 / 4**（全模式） | 200 | 27 | 0 |
| B52 static / fast / slow / hold | **14 / 8**（全模式） | 800 | 134 | 16 |

- **13 盘 reason 10/stage 4**：`wallHit` bracket 返回 `unresolved`（flags=4）——解析分段求值无法给出符号确定的穿壁根（几何门）。
- **52 盘 reason 14/stage 8**：Jacobi 接触求解后 `z < -residualSpeed` 或 `complement` 超界——求解器残差超容差（收敛门）。
- 无任何接受根：typed 接受、守恒/接触残差正例、同窗 shader 原件不可达。每臂原子拒绝原字（completions/G16_COLD/base/candidate）全档。
- **前置**：developer 调物理（bracket 求值策略或求解器容差/迭代）；G16-C 按 T938 合同继续等 B 正例。

## ③ 活动槽原子拒绝注入（T936 夹具合同）——全过

| 臂 | 注入 | 结果 |
|---|---|---|
| g16-bad-first | slot 1 `flags=0`（首根） | `passed=true`；outcome/reason/stage **2/2/1**、stageMask **0x801**、firstDisk=1 |
| g16-bad-second-bank | slot 13 `flags=3`（第二根/bank 换后） | exit 3——**合同行为**：首根本身物理拒（reason 10），“故障之前若有物理拒绝则保留原首错”，注入未达 |
| g16-bad-late-slot | slot 52 `reserved1=1` | `passed=true`；2/2/1、0x801、firstDisk=52 |

- 共同核证：**BASE 不变**（trustedBaseSHA 与 faultyBaseSHA 仅注入字不同）、无发布（acceptedRoot=null）、同 fence 退役、冷原字导出（G16_FREEZE_FIXTURE_BYTES 带 base 副本三 SHA）。

## ④ 负控三组——全过

| 控 | 结果 |
|---|---|
| 变造种子（翻 1B）→ 正常 PE | exit 2 `G16_SEED_SHA` |
| 旧模块（0f9b9e75）→ 新 PE | exit 2 `G16_MODULE_SHA` |
| 新模块（786bb1db）→ 旧 PE | exit 2 `G16_MODULE_SHA` |

## 结论

| 子项 | 结果 |
|---|---|
| G16-A | 复用 T932（过） |
| G16-B 正例 | **受阻**：8/8 物理层原子拒绝（13 盘 bracket unresolved、52 盘 solver 残差）；T936 修复生效但正例需 developer 物理侧跟进 |
| 活动槽原子拒绝 | **完成**：首根/后槽注入全 2/2/1+0x801+BASE 不变+无发布；second-bank 臂按合同保留物理首错 |
| 负控 | **完成**：SEED_SHA/MODULE_SHA×2 全拒 |

GPU 驻留、shader 渲染全程；私有根真实 250ms 门原值。
