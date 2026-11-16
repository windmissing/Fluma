# T513 · GPU 起沙守恒与来源独立反例基线 tester 报告 v1

- 卡：`6af8605ce4b0379c7edb1c86`（tester，P5，不使用真机）
- 日期：2026-11-16
- 依据：algorithm-T506-gpu-lift-suspension-v1.md §2–7 + design-T506-gpu-lift-fixtures-v1.json（28 输入）
- oracle：test/t513/lift_lineage_oracle.py（独立整数推导，不 import designer 实现）
- 产物：test/reports/t513/t513-lift-lineage-oracle.json

## 结果 29 PASS / 1 NOT_IMPLEMENTED / 0 FAIL

### 来源层与顶 cohort（LU01）

- TOP_SOURCE_ONLY：只取实际顶层（origin2 mass7），granted=7≠总量口径 9——错误 origin 借总守恒（107≥9）被拒
- LAYER_FULL_DIFFERENT：64 层满且顶不同 → LAYER_CAPACITY_WAIT 零扣费（独立推演 layer append 状态机）
- LAYER_FULL_SAME：顶同 origin 合并 depth 仍 64、origin mass+5
- ORIGIN_SWAP_TOTAL_SAME：bed=[{1:50},{2:57}] vs 伪造=[{2:107}]——总 107 相同但逐 origin 审计 1:−50 精确拒绝

### 守恒与标签（LU02/LU03）

- PARTIAL_LAND_TAG：独立推导 (m,tag,q)=(6,3,1) → l=floor(3/6)=0、remMass=5、remTag=3（与 designer expected 逐字合）
- FULL_LAND_TAG：必返全部标签 3、mass/tag 归零
- EXTERNAL_A_LIFT_B_MERGE：保 a 证书/来源，tag 相加 4+0=4、mass=9
- U_NOT_DOUBLE_COUNT：P=U+A=14、T=14/eta=19/R=21 独立复算全合
- MEDIUM_ONLY_RELABEL：A−11/U+11 成对、P/S/I 不变
- LIFT_OCCUPIED_INVARIANT：S−q/P+q 后 T/eta/R/W/J 精确不变（23/28/30）

### 能量与请求（LU04/LU05）

- OLD_ENERGY_REQUEST：request 用 E0=0（本根 qReq=0 零物质）、E1=65536——rampQ16 需 ≥120·65536 才能一步到 s（ramp 具体值属 §9 待裁决，oracle 用合同支持域内值验证公式）
- SENSOR_CALLBACK_COUNT：一样本 120 quanta 每步 E1 单调爬升（120 步 0→65,520），不按 callback 加量
- STALE_NO_DECAY_STEP：stale 暂停 tick 不积分 E（区别于正常零输入有理衰减）
- ROLE_ENERGY_PURITY：5 角色各从相同 E0 得相同 E1（65536），不累计 5 次加能
- POOL_FULL：E0=Q16 120、band=100 → qReq=100（请求>0，池满由容量门拒、无隐藏 U 回退）
- GENERATION_OVERFLOW：溢出 fatal 丢根（T497 fatal 合同承接）
- GUARD_EDGE_EQUAL：边界相等 GUARD 拒（designer fixture 闭域语义记录）

### 事务与谱系（LU07/LU08/LU09）

- SOURCE_TRIAL_ENERGY：8 trial 零物理时间不触发 lift/E；失败 trial 证书/U 回滚
- SOURCE_Q1_Q2_PREFIX：SOURCE→Q1→Q2 失败保前两根（T497 §6 原子尾承接）
- AUX_STREAM_MISSING：物理 cut 缺来源片 fatal、不事后补造（writer 封闭矩阵）

### 输入/时钟/配置（LU06/LU10）

- NATIVE_RECEIPT_IS_NOT_CAPTURE / CLOCK_DOMAIN_UNPROVEN / SENSOR_AGE_BOUNDARY（0≤age≤250ms 相等边界允许）
- PRODUCT_DEFAULT_OFF / NO_T22_ADVECTION / CONDITIONAL_RETURN / LATEST_REAL_ANGLE（±30° 域）
- HIGH_PEAK_IMPOSSIBLE：**峰值占比精确分数 122333/5242880≈0.0233 < 0.5**——2.33% 支持域与旧 50% 缺口保持（合同 §9）

## 判定域分列

- **oracle/输入冻结**：PASS（29 项；5 处初版参数/构造错误均独立复算修正，修正记录在脚本注释）
- **产品 V5 GPU 执行**：NOT_IMPLEMENTED（真实资源/时钟/设备——设计算例不作 GPU 运行，不虚报 PASS）
- **layout 依赖项**：等 T512 机器表后接（合同允许，不阻当前语义基线）
