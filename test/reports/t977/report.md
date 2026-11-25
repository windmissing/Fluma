# T977 · T975 单角色资格、T976 面通量与 Demo local 合成集显实测（effector）

2026-11-25 · 开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924）。GPU 驻留、shader 渲染，无 Canvas 回退。注：用户发 "977" 时滴答尚无 T977 卡（search 无果）——按最新交付 T975/T976 的 effector 实测范围执行，报告编号 t977 落盘。

工件全核：T975 资格包（PE `c27e3742`、manifest `f6ce86b1`）；T976 双包（dm2 PE `873f9c98`/roles `721e5c09`、Demo PE `21bce811`/manifest `94a9436b` 5/5+启动器 local 四模式）。

## ① T975 G13 单角色资格——5/5 过（关键修正）

| 角色 | opt 臂 | noopt 臂 |
|---|---|---|
| OP55（20,129B GLSL） | pipeline EXIT vk=0（**85.4s**） | vk=0（0.4s） |
| OP58 | vk=0（80.9s） | vk=0（0.4s） |
| OP56（26,615B） | vk=0（**94.5s**） | vk=0（0.5s） |
| OP59 | vk=0（106.6s） | vk=0（0.5s） |
| OP_SOURCE_DRIVE_ONLY | vk=0（105.9s） | vk=0（0.5s） |

- **关键修正**：T968 判定的 OP55_58_INSTALL/OP56_59_AUDIT"挂死"实为**编译时长超出 120s 观察窗**（singleton 拆分后 85–107s 完成）——singleton 拆分（T973 建议路径）实测有效。
- `--disable-optimization` 臂 0.4–0.5s 完成（优化 pass 占约 200 倍耗时）。
- 四旧角色（OP55_58_INSTALL/OP56_59_AUDIT 组合版 + OP_48/OP48_REMAINDER）180s 仍超时——组合/大模块确认为真超长编译或挂死。
- OP66 探针在资格入口未见行（T975 文档：OP66 证据对应角色资格、SOURCE 线需实际 INPUT）。

## ② T976 面通量等价候选 ABBA——等价过、单次性能不判定

- ABBA 四臂（A1/B1/B2/A2，`--quantum-detail face`）：`selectedJ=1`、首错 (0,0)、`faceExactDivide` False/True 标记正确；`T976_FACE_DIVIDE_BINDING` 2 行（opcode 25 `b4e8c61a→cac34472`、opcode 27 `3992c136→d0d7adf7` 模块替换确认）。
- **等价**：EVENT/PATH 逐字同；差异词仅身份/时钟/seal 区（与 T954/T968 同构分布）。
- 27 GPU 点（15 粗+12 face 边沿）、同 fence 4/4。
- 整根 span：off 1071/906ms vs on 1057/1001ms——单次对照中候选收益不可从 iGPU 波动（±10%）分离，按 T976 文档需配对重复（如实登记为单次不判定）。

## ③ T976 Demo local 合成——过（T968 阻塞解除）

`--m1-demo-synthetic-local`（SYNTHETIC_LOCAL token，免前台）三臂：

| 臂 | 合成行 | presents | admitted |
|---|---|---|---|
| local-stationary | 2（DOWN (312,215)+UP，4.007s 持按） | 647 | 13,803,016 Q（active 0 全落床） |
| local-move | 120（DOWN+**118 MOVE**+UP） | 727 | 15,067,760 Q |
| reference-local | 2 | 714 | 5,513,872 Q（reference 调度差可见） |

- `M1_DEMO_SYNTHETIC_LOCAL_INPUT` 字段全录：inputDriver=SYNTHETIC_LOCAL、fixtureHold token、`osCaptureOwned=false`、**`foregroundMatched=false`（如实记录不依赖前台）**、viewRevision/authorityEpoch/sample。
- **T968 的前台门阻塞被 T976 local 路径实测解除**；真实输入路径仍按合同走原门。

## 结论表

| 子项 | 判定 |
|---|---|
| T975 单角色资格 | **过 5/5**：singleton 拆分有效（85–107s 慢编译）；T968"挂死"结论修正 |
| T976 面通量等价 | **等价过**（业务字节相等+绑定 live+同 fence）；单次性能不判定（需配对重复） |
| T976 Demo local | **过**：免前台合成三臂全通，T968 阻塞解除 |

证据根 `test/reports/t977/runs/`（qual 10+4 jsonl、face 四臂 raw+jsonl、demo-local 三臂）。
