# T249 V2 落床与低帧率定向补证报告（v1）

日期：2026-10-29；tester；**真机 VYG-AL00（62T0225B10010020）**。被测：T248 诊断包（`T248-signed.hap` SHA `9886b457…`，页面 P051DerivedV2Demo，XComponent+shader）。原卡 `6ae28fade4b0f11bdf375858`。按 T243 精确矩阵执行，不重跑 T218 全矩阵。**结果回原 T243 architect 分流，不直接 developer 修热点。**

## 结论

**BLOCKED（主锚点）+ 2 项关键缺陷证据**——源注入 8.255s（≥8s 达成）后，**首次 LAND 从未发生**（床量恒 0），且在停源观察期主线程被物理 step 阻塞触发 **THREAD_BLOCK_6S appfreeze**，应用冻结后无法采集"再 3 个完整 CURRENT 帧"。依据任务书规则：未达成矩阵锚点按 BLOCKED 上报，**不据墙钟签落床失败**——但本次已有逐帧诊断证据链支持首次归因。

## 矩阵锚点执行状态

| 锚点 | 状态 | 证据 |
|---|---|---|
| 出生 | ✅ 达成 | run `p011-run-1793248756099-3`（pid 1806），24 次注入全部 `ok=true rej=none`，admittedH 累计 **50.099 单位** |
| 20s 墙钟 | ✅ 达成 | 注入窗口 12:41–12:42（keep 60s 持续按压）+ 停源观察期，远超 20s |
| 累计接受模拟 ≥8s | ✅ 达成 | T248_SOURCE 累计 **8.255s**（requestedH=8×sec）；T248Observation cumulative 43.53s（跨 run 累计器，本 run 接受模拟另计） |
| 首次 LAND 床量变化 | ❌ **BLOCKED** | T248_LAND 逐帧 events 全 `[]`（100/100 帧）；publication `bh=0 bl=0 mh=0 ml=0`、`bedColumns.bh[128]` 全 0；粒子 states=[0,512,0,0]（512 粒全非床态），minY=124.44 / maxY=127.11（**全部粒子悬浮在 y≈124–127，无一带床**） |
| 再 3 个完整 CURRENT 帧 | ❌ **BLOCKED** | 停源后主线程阻塞，**THREAD_BLOCK_6S appfreeze**（12:42:35 watchpoint，12:42:48 Faultlogger 合并 freeze 文件），帧号停在 1307；冻结后仅可采到冻结前同一帧（commit 2637） |

## 关键发现（首次归因证据，回 T243 architect）

1. **粒子下落速度异常缓慢/停滞**：注入 50.1 单位沙后全部 512 池粒子（含注入粒子）y∈[124.44,127.11]、vy∈[-7.50,-3.39]（模型 256 高度域），长按 60s+停源期间无任何粒子触底。vy 为负（向上为正坐标系下向底沉降）但位移每帧 <0.1 单位——与 T218 "dry 粒子不落床" 结论一致，且本次有逐帧 y/vy 原始序列佐证。
2. **0.19fps→0fps 退化路径定位**：帧间隔实测 5.2s/帧（12:33:53→59→12:34:04→19，**≈0.19fps**，比 T218 记录的 0.5–8fps 更差），随后主线程完全阻塞触发系统 freeze 检测。renderInclusiveMs 仅 37–46ms——**耗时大头不在渲染，在 JS 物理 step/observation 采集段**（stepMs 字段在冻结前帧未见异常值，阻塞发生在冻结帧的 step 内部，无正常回执）。
3. **appfreeze 发生在停源后首个 step**：最后一次成功注入 12:42:28 附近，最后 FRAME=frame1307/commit2637 于 12:42:37，watchpoint THREAD_BLOCK_6S 于 12:42:35 登记。冻结时 heap：TOTAL 23.6MB / OBJECT 18.0MB（无内存耗尽迹象）——指向**计算阻塞而非资源泄漏**。

## 路由语义确认（非缺陷，工程事实）

- T248 生成器 EntryAbility patch 落空（主仓已无 `'pages/Index'` 字面量），冷启动默认落到 `pages/SandCanvas`（errorCode 13 加载失败）。**运行时用 `--ps p051v2 1` want 参数路由到 P051DerivedV2Demo 成功**，诊断流完整工作。此项为 T248 交付工程注记，不阻塞取证。
- 触摸注入须用 `uinput -T -m x y x y -k <ms>`（Down 保持）；`uitest longClick` 每次仅 ~0.2–1s 且 Up 触发 stop()，`uitest drag` 在 SURFACE 上不触发页面 onTouch。已用 uinput keep 达成 8.255s 连续注入。

## NOT_DONE / 边界（如实）

- 首次 LAND 床量变化、再 3 CURRENT 帧：**BLOCKED**（appfreeze + 无 LAND），未采得。
- p50/p95 分段计时：帧样本不足以统计（冻结前有效帧 ~35 帧分布于 3 次拉起，原始序列已归档但分段统计无意义），不编造。
- 像素采样、publication 拷贝后同列对比：publication bh/mh 全 0 与 packed data 头部 0 一致（`T248_PACK data:[0,0,...]`），像素级对拍无对象可对。
- 本 run 累计模拟秒：cumulative 43.53s 为跨 run 累计器值，本 run 内接受模拟秒未单独冻结快照（冻结阻塞），以 SOURCE 8.255s 与帧 dt 序列为准。
- 真实 GPU 收益/产品 M0：不涉及（T248 是 CPU_ORIGINAL_F64 路线 V2 隔离观测）。

## 证据

- `t249_inject_all.txt`：24 次注入原始行（seq/sec/ok/adm/rej）。
- `t249_tail400.txt`：冻结前 400 行 T248_FRAME/STEP/LAND/PACK 原始 JSON。
- `t249_freeze.txt`：THREAD_BLOCK_6S/3S 与 Faultlogger 时序。
- `t249_f2.txt`：冻结前最后一帧完整 FRAME JSON（publication bh/mh=0、states、minY/maxY/vy）。
- `t249_cur1-3.txt` + `t249_ws1-3.txt`：CURRENT 帧（冻结前同一 commit 2637）与 WS_EVENT。
- `t249s.jpeg`：页面截屏（渲染区中央单粒白点可见，床不可见）。
