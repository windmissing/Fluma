# T218 数值 V2 双态沙真机验证 · DD12 报告 v1

- 角色：tester（真机 VYG-AL00，串行）· 日期：2026-10-28 · 状态：**BLOCKED（证据交 architect）**
- 被测包：**s142**（构建于 T216 4d3584d 树 + T217 后，含 p051v2 页面路由最小增量 + T218DBG inject 打点；unsigned 235dae25→s141→s142 两次迭代）；旧 s140 同树先行验证
- 页面：`pages/P051DerivedV2Demo`（T216 显式新 run 隔离入口；生产 Index/V1 默认入口未切换）；facade 配置 `isolated=true + DERIVED_POLICY_V2 + CPU_ORIGINAL_F64 + T216_DRY_WRITERS_V2 + nmax=512`
- 归档：`test/reports/t218/`（t218-idle/inject1-6/hold8-9/tilt/3click/reset/s142-inject/late.jpeg + layout2/3/4.json）

## 已证实（真机证据）

1. **V2 新包运行**：页面标题"空容器落沙 · 数值V2隔离"、状态"运行中"、runId=`p011-run-1793200872692-3`。
2. **注入链路正常**：uitest 长按触发 touch→model.start→clock；T218DBG 日志 11 次 inject 全部 `ok=true, adm≈0.54–1.34 mass/次, rej=none`（无一次拒因）。reset 后沙粒从屏上消失 → 渲染内容确为 V2 publication 活内容。
3. **AIR 粒子可见**：注入点正下方出现沙色像素簇（y≈714–742 屏幕坐标 ≈ 出生点），同 FrameToken 渲染成立。
4. **CURRENT 精确**：WS_EVENT p013_scene 持续 `state=CURRENT, swap=true, staleSwap=false, first=NONE, configurationFailure=""`（frame 204→236，commit 421→485，每帧+2）——无 FAULT/STALE 回退，无 Canvas。
5. **物理 step 在跑**：commit 持续递增（0.5fps 慢帧率下仍每帧两 commit）。

## BLOCKED：DD12 床沙可见未达成

- 长按注入 ~50+ mass（≈500 粒）后 20 秒，屏幕仅有出生点附近 4–5 个沙色采样像素；**无床沙堆渲染**（粒子落床后应 LAND 抬 S/bh，床高≈48 单位，应在画布底部可见）。
- 排除项：非注入失败（adm 全成功）、非渲染冻结（frame/commit 持续递增、swap=true）、非重置残留（reset 后清空）。帧率 0.5–8fps（XComponent TEXTURE GL 通道极慢，独立性能问题）。
- 定位分歧点（超出 tester 修复权，交 architect 裁决）：
  - **A 假说**：V2 publication 的床沙通道（height-field/bh 投影）未进 P013 渲染路径——T217 离线已证 tuple writer/LAND 正确，若 publication copy 后 bh 非零而屏上无床 → 渲染侧缺床层绘制；
  - **B 假说**：dry domain 粒子在该配置下从未 LAND（保持 AIR 停在空中 y≈225——与 s140 那粒静止位置吻合），则涉及 P051 dry 物理策略。
- 区分两假说需要 publication 内 bh/mh 数值 dump（demo 未打该日志；加 dump 属代码改动，tester 建议由 developer 在 architect 定向后补）。

## 其它如实记录

- 帧率 0.5–8fps：不满足任何产品/演示可用门槛；与 T225/T232 的 pair 桥 1380ms/64root 不属同一通道，属 XComponent 逐帧全清渲染性能，需另立性能卡。
- s140→s141 两次"只见 1 粒"的初步误判已由 T218DBG 日志纠正：注入一直成功，是帧率+床渲染共同造成低可见度。
- uitest 连续第二轮 drag 的 Down 未达 App（0 新日志）——测试注入局限，不影响结论。
- 旧 V1 包/V1 错误现场未复用；本卡全部新 run。

## 失败上报

按卡约定：**DD12 不达成 → BLOCKED 先 architect**。不回填其它卡，不改清单其它行。
