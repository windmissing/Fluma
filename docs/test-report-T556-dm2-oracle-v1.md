# T556 v1 · 目标9动态动员v2 独立反例与动力学基线（本地语义域）

日期：2026-11-17，tester，不使用真机/GPU（T555 实现未交付——GPU 域全 NOT_RUN）。
依据：algorithm-G9-dynamic-mobilization-v2.md（§4 整数公式/§9 R0 参数/§10 32 fixtures）
+ design-G9-dynamic-mobilization-fixtures-v2.json。
证据：`test/t556/independent_oracle.py` + 本报告。

## 独立性声明

`test/t556/independent_oracle.py` 从设计文档 §4 权威整数公式**独立实现**期望值推导：
自写 RNE signed round-half-even、floor 非负除、Eval(q)、闭式上界、32 轮 lower-bound
求解器。**不 import designer 的检查程序**（design-G9-dynamic-mobilization-checks-v2.py
按 T554 边界仅作对照源，不作期望来源）。

## 独立期望值结果（7 个可本地推导 fixture）

| fixture | 独立结果 | 判定 |
|---|---|---|
| DM01 GEOMETRY | chiG∈{0,1/64,1/8,1}（gn=512cos30=443.4Q）：qPhysical=0/4610/57795/1587033，**非减且首零**，H0/omega0 随 chiG 响应（131072/1048576/8388608；286720/458752/1835008） | 与 fixtures expected "zero then nondecreasing; H and omega respond" 吻合 |
| DM02 ORIENTATION | gx=g·sinθ、**gn=g·cosθ**（法向重力）：0°/15° chiG=0（stable），30° **chiG=81107 (0.077348Q)**、qPhysical=30192 | **chiG30 独立复现设计 §9 数值 0.07735（差<0.001）**；"30 geometric candidate positive" 成立 |
| DM05 POSTMIX | F=3,p=12,q=3：u 4→2、动能 48→24、p 逐位不变 | velocity drops / no velocity gift / ke drops 全部成立 |
| DM06 TARGET_FEEDBACK | toy(omega=8,gap=10,den=8)：可行集 {0..5}，**max q=5，q6 不可行** | 与 expected "max feasible integer q5, q6 infeasible" 逐字吻合 |
| DM07 SOURCE_PREFIX | qPhysical=9、bed top-first [[1,4],[2,5]]、仅 origin1 在场：qOrigin=4、qActual=4、不跳 origin2 | 与 expected (4,4, no jumping) 吻合 |
| DM19 ACTUAL_BASE | entryP=3+actualImpulse=4=**7 而非 10**；pMid=6 仅诊断 | actual 从 pEntry 累加语义成立 |
| DM17 BASE_KICK_COUNTEREXAMPLE | **ORDERING 级反例独立确认**：均匀 S40/F=p=P=0/30°——旧序零通量+重力源增 p>0+势能不变+角度外功已在快照消费 → 严格能量门必拒，无 ε 放行空间 | 与 designer evidence "ANALYTIC_UNIFORM_CONSTRUCTION_NOT_GPU_FAIL" 两端独立一致 |

## 与 designer 检查程序对照（只对照，不作期望来源）

- designer evidence 全部检查 PASS（1548 局部单调性点/180 有限集求解/3 闭式/postmix 反馈）；
  其数值点（如 u0=25165824）使用与 fixtures DM05 不同的参数点，与我的独立值**无冲突域**。
- DM17 反例两端独立确认（名义级+ORDERING 论证级）。
- 我方 DM02 发现：gn 必须按**法向重力 g·cosθ** 取值——设计 §3 的 gx/gn 分解在 DM02
  fixture 语境下是唯一自洽读法，并使 chiG30 与设计 §9 的 0.07735 精确吻合。

## NOT_RUN（GPU/产品域，待 T555）

- DM03/04/08–16/18/20–32 中需 GPU 真实执行的域（D10 全管线、replay policy、CFL 重核、
  冷预算）——**全部 NOT_RUN**，本地语义域不冒 GPU 实测。
- DM20 强起动全管线、DM22 replay 拒收、DM29 动员后 CFL、DM31 冷预算——T555 完整 GPU
  实现后按同一 oracle 通道续测。

## 判定

- 设计 §4 整数语义、§6.1 反例、§9 R0 数值量级：**独立可复核**（tester 通道成立）
- DM17 起动兼容反例：两端独立确认，纳入 T555 实现验收硬门（旧序能量门拒绝必须保留）
- 本卡核心目标（独立反例与动力学基线）在本轮可做域内**完成**；GPU 域随 T555 交付续作
