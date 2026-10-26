# T47 规划阻断补证 v1

日期：2026-10-26。责任：tester。任务：[6addc614…33fe](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6addc614e4b017ab33eb33fe)。状态：**EVIDENCE_COMPLETE_FOR_ATTRIBUTION / ROOT_CAUSE_BELONGS_TO_DESIGNER / NO_BUSINESS_CHANGE**。

按 T46 归因稿 §4 tester prompt 完成离线补证：step73 最小复现 + blockedReason/回滚逐值证据 + 四个对照臂。执行器 `test/p047_tilt_blocked_evidence.cjs`，原始逐值证据 `test/reports/p047_tilt_blocked_evidence.json`（含失败前后各 10 步逐列/逐粒/预算/控制量全量记录）。不使用真机；不改业务、不改判据。

## 0. 最小复现与基线对照

| 项 | 值 |
|---|---|
| 场景 | 128 列、平床、nmax=16、同列 (x=64.5) heavy 注入 1.6、seed=1、dt=1/60、≤360 步 |
| 当前工作树复现 | 原 probe 重跑：step index **73**、ok=false、stepped=false、substeps=0、errorCode=**5**、errorColumn=-1（与归档 baseline `2ef21f9` 逐项同值） |
| 池终态 | 11/16 全 resting，heavy=1.0226497308103744 |
| 池占用历史峰值 | **16/16（满池）**，出现在 step ≤70；step 71 首次 LAND（16→12）、step 72（12→11） |
| 漂移 | water≈2.05e-12、H/L=0（与 baseline 一致） |

## 1. 失败逐值画像（step 73）

- `planner.status='blocked'`、`blockedReason='NO_SAFE_OLD_TILT_PLAN'`、`passCount=2`、`fallbackUsed=true`、`thetaRequested=0`（=thetaBefore，无新请求）、`limitingEdge=null`、`configRevision=0`；
- 失败前后**状态逐字节不变**（stateDigest 含全部床/水/悬沙/池数组、nextId/freeSlots、physicsCommitId）——无半步提交、无 ghost 写入；
- **field 未冻结**：失败后再次 step 仍返回 code5（非 frozen 死锁），与 §20.4「BLOCKED 不永久冻结」一致；
- 触发链：step 71–72 两笔粒子 LAND 把 col63/64 床沿推到**恰好 tan30**（0.5773502691896262，Float64 上比 `P011_PLANNING_PASS_TAN_HARD_MAX` 常量高 5e-16，在 1e-12 容差内不构成 preexistingViolation）。

## 2. 四对照臂分流（正常容量拒绝与 TILT_PLANNING_BLOCKED 分开）

| 臂 | 设置 | 结果 | 分流意义 |
|---|---|---|---|
| 无注入平床 | 同场不注入 | **360 步全 ok**，planner 全 accepted | 注入链（粒子 LAND 堆床）是失败因果前提 |
| 同注入 nmax=512 | 容量余量 16→512 | **仍在 step 73 code5、同 blockedReason** | **排除池容量拒绝**——不是 CAPACITY_OVERFLOW，也不是 slot 不足 |
| 同注入停输静置 | 在 step73 前清队列只步进 | **第 0 步即 code5** | 无新输入仍阻断——**状态驱动阻断**，非输入请求触发 |
| 同状态重复步进 | 失败态再 step（θ=0） | 仍 code5；但 **setTilt(5) 后同状态 accepted**（limitingEdge=64，即床沿列） | 同状态对 θ=0 预览不可行、对小倾角请求可行——阻断仅限于"旧角回退"路径 |

**全程无任何容量拒绝（CAPACITY_OVERFLOW=0），TILT_PLANNING_BLOCKED=1 次且持久**——两类拒绝未混计。

## 3. 机制定位（可判定部分）

1. step 72 的坡度松弛把 col63 床沿精确压到 30° 安息角（S 差 = tan30）；
2. step 73 规划进入时 `preexistingViolation` 为 false（残差超常量 5e-16 < 1e-12 容差）；
3. blockedReason=`NO_SAFE_OLD_TILT_PLAN` 且 passCount=2、fallbackUsed=true → **PASS1（θ=0 预览）与旧角回退（同为 θ=0）双双返回不可行**；θ=0 预览在该床沿状态下不可行的机制在两处：preview `plans=null`（粒子 apply 非 0 或 preview substep collect 报错）或 `p011DecideCandidate` 判区间不可行——**现有遥测不含 preview 内部失败原因，二者离线不可区分**；
4. `limitingEdge=null` 而 θ=5° 请求 accepted 时 `limitingEdge=64`——可行动的边界证据指向同一床沿列。

## 4. 交 designer 的判定材料与观测请求

**归 T46 §2 分流框架**：
- 「正常容量拒绝」已排除（nmax512 同失败）；
- 「预期规划阻断 fail-closed」与「算法/实现缺陷」的界线取决于：**θ=0 预览在"床沿恰为 tan30 安息角"状态下的不可行是否为契约允许**——床沿在无倾角时到达安息角是松弛算子的正常产物，若契约认为该状态可行，则 planner 在可行态持续阻断指向规划算法/边界契约；若契约允许该状态被阻断，则属预期 fail-closed。此判定归 designer。

**请求补充观测（现有遥测缺字段，需 architect/developer 提供后才可闭合机制层）**：
1. preview 内部失败原因（`plans=null` 时区分 particleStatus≠0 / substep collect errorCode）；
2. preview 两 pass 的 `p011DecideCandidate` 区间值（loSlope/hiSlope per edge）；
3. 失败步的 `substepSandTopBefore` 完整数组（可从观测端补采，当前探针已留 stateDigest）。

## 5. 边界

- 不修业务、不改判据、不猜根因（§3 的两分支如实列为不可区分）；
- 观测探针只读公共 API 与 publication，未 patch 业务对象（唯一注入点是 developer 语义内的 sink/audit 覆盖，用于 D 类回滚验证，均已恢复原绑定并验证）；
- 不使用真机；T44/T66 的采集/落盘工作与本稿无文件交集。

## 6. 变更记录

- 2026-10-26，v1：首次发布。step73 在当前树与 baseline 同值复现；blockedReason=NO_SAFE_OLD_TILT_PLAN（passCount=2/fallbackUsed=true/θ=0）；失败步状态逐字节不变、field 不冻结；四臂分流排除容量拒绝与输入触发；触发链定位到 step71-72 LAND 把床沿推至恰 tan30；机制层两分支（preview null vs 区间不可行）因缺 preview 内部遥测不可区分，三项观测请求交 designer。
