# T520 · 目标9 双向动力学独立用例与反例基线 tester 报告 v1

- 卡：`6afa5284e4b0711ec48fdd9f`（tester，P4，不使用真机）
- 日期：2026-11-16
- 依据：algorithm-T515-gpu-three-state-dynamics-v1.md §4–7 + design-T515-three-state-fixtures-v1.json（43 输入）
- oracle：test/t520/three_state_oracle.py（独立整数推导，不 import designer 实现）
- 产物：test/reports/t520/t520-three-state-oracle.json

## 结果 45 PASS / 1 NOT_IMPLEMENTED / 0 FAIL

### P→F capture（§4.1）

- G903_CAPTURE：整包捕获 `P−m,F+m,p+=m·vx` **逐字**（newF=1572864、newP=4398046511104）；
  m·vx=524288·4194304=2199023255552 独立算术复核；竖向势能不变（新层质心=B+m/2）
- G904_WRONG_CONTACT_CENTER：y=B 直接摊平会凭空升高质心（m²gn 增）→ 拒
- G921_EMBEDDED_AFTER_CAPTURE：捕获抬床吞其它包 → 拒 EMBEDDED_PACKET 不 finalSand 上推
- G937_HORIZONTAL_CAPTURE_PROJECTION：ΔU2=−2gx·m·(xC−xP)=+Q³/4>0 独立复算、无碰撞耗散可抵 → 拒
- G938_NO_DOUBLE_CONTACT_TIME：P 只推进到接触 tick、capture 占末微步、新 F 下微步才动

### S→F 动员（§4.2）

- G905_ACTIVATION：S−q/F+q/p 不变（静止床初始动量 0）
- G906_NO_INCOMING_SAME_SPEED：赋同速动能翻倍被拒——混合减速是实际耗散
- G907_SHEAR_RECRUIT：u=RNE(p/F)=10Q、dShear=RNE(4096·u²/(65536·1Q))=6.25Q > static 0.5Q（惯性剪切闭包独立复算）

### F→S 沉积（§4.3）

- G908_STOP_DEPOSIT：p=0/rest 32≥32 → 全部静止 F 按来源键升序入床
- G909_MOVING_NOT_DEPOSIT：p=1≠0 不沉积+计时清 0（停源≠动量清零）
- G910_DEPOSIT_LAYER_WAIT：64 层满且顶不同 → REST F 保留
- G942_NEW_MASS_RESETS_REST：捕获清 restTicks=0

### F→P 发射（§4.4）

- G911_LAUNCH：vx=TZ(p/F)=3Q、px=q·vx、newP=4947802324992、y2=2(S+F−q)+q=7864320 **逐字**；
  质心等高势能不变独立复算
- G912_SPLIT_ROUND_ENERGY：F=3,p=1,q=1 → K2 1/3→1/2 涨 1/6 必须入功上限或拒
- G913/G914：Jy=0 无自由发射；POOL_FULL 不发射不改账

### F 运动（§5）

- G902_STATIC_FLOW_MASK：Bs=muStart·gn·F/(65536·120J) 静止掩码 q=0（Rusanov 不偷搬质量）
- G915_TWO_OUTFLOWS：两面 Hamilton 逐来源合计 [1:3,2:2]（总量法独立复算）
- G916_HAMILTON_TIE：余量按分数降序+来源键升序（1/2 各+1，无原子抢占赢家）
- G918_PRESSURE_Q_ZERO：q=0 时 jp 仍传力（不强行解释成 q·u）
- G919_FRICTION：REST |−12|>Bs10 → p′=−5（proximal）→ FLOW
- G920_WEAK_IMPULSE：REST |8|≤Bs10 → p′=0、supportImpulse=7 入环境账
- G933：F=0 且 p≠0 拒 trial（不除零/不消残动量）

### 能量/试次/封行（§6–7）

- G924 角度外功一次性入账、G925 相同项符号抵消 ΔE=0、G926 区间重叠 → ENERGY_RESOLUTION 细分
- G927_FIRST_SUCCESS：CFL/ENERGY_RESOLUTION 失败后 J=4 首成即选中（非 8）
- G928_ALL_BLOCKED 零物理提交、G929_PARTIAL_SEAL 不推进前缀
- G934_OVERFLOW：S128 累计满 +1 → FAULT 不回绕
- G943_NEGATIVE_WORK_NO_CREDIT：正功 9>cap8 且负功不抵 → 回滚该列触摸

### 输入/版本/源（§7）

- G930_OLD_READER 版本拒、G931_STALE_INPUT 旧传感器不冒 V5 输入
- G939 dedup ack 消费 body、G940 +0/−0 位型不同 → BAD_DUPLICATE（全 body 原位比）
- G941_SOURCE_CAPPED_ADMISSION：admit60/reject40 单来源记 60

## 判定域分列

- **oracle/输入冻结**：PASS（45 项；1 处初版值类型错误（JSON 字符串数字）修正）
- **产品 V5 GPU 执行**：NOT_IMPLEMENTED（真实 run 域；模型/契约与 GPU 运行分列）
- **工程 wire 依赖**：T524 工程闭包（architect 在途）——语义基线不依赖


---

## 勘误（2026-11-17，T539 卡面要求：G920 supportImpulse 文字核）

G920_WEAK_IMPULSE 报告正文原写 "p′=0、supportImpulse=7 入环境账"——**7 是误值**（Bd=7
的动摩擦参数被误当反冲量）。按 T515 §5.8 正式式：入口 REST 且 |pTrial|≤Bs → p′=0
（静摩擦分支，不经 Bd）；"差额进入床/容器反冲" = p′−pTrial = **0−8 = −8**（环境账收 −8）。
oracle 数值断言（pAfter=0、modeAfter=REST）不受影响——错误仅在报告文字，fixture/oracle
无改动。原件（本报告勘误前版本）留存于 git 历史（commit 4e2647f）。
