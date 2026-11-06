# T392 · CPU Z08 发布与 LAND 同帧冷档

隔离签包由 T381 生成器派生，启动进入 P051 Derived V2 的 XComponent + P013 新管线。签包和 SHA256 见[签包回执](../t257/T392.json)。设备取证交 T393。

在隔离页填入 1–8 个已知粒子 ID，点「预约下一帧」可冻结下一帧的选中子步和跨列 companion。点「预约 LAND」等待第一个包含选中 ID 正授予 T3 的帧，上限 2,400 个回调。点「近床夹具」会重置隔离场、预约 ID 1/2/3，并从原干沙准入入口投放 0.3 单位。持续试验应先保留原始受限臂，再单独启动近床夹具。归档完成后点「导出 Z08」，文件名和 SHA256 同时在页面和 `WS_EVENT t392_z08` 出现。一个预约只冻结一份归档，文件写入前已在同步帧回调中冻结。

- `candidates` 是当前回调内最多两个实际成功子步。每项含 ACTIVE seal、certificate/content serial、run/epoch/commit/source epoch、原 ledger key、dt、实际角度和筛选后的原跨列 companion。被后续子步覆盖的候选为 `NOT_SELECTED_FOR_DRAW`。只有与当前复制槽完全一致的最后候选会进入呈现配对。
- `PRESENT_CURRENT_SWAP` 要求该调用直接返回的 P013 收据为 CURRENT、请求和实际 token 等于当次 native frame token、请求交换成功，且提交、ledger key、场/表面/context/几何/连续性 epoch、配置与代次均一致。原收据随档保存，`systemPresentVerified` 单列。STALE、错身份、无 draw 和非本回调 publication 都有独立结果。
- `lands` 从实际 Hot T3 事件采样，仅在原提交边界完成后归档。事件保留 ordinal、slot、ID、generation、source、列、requested/granted/remainder、行前后砂床、local/total 扣减和原票据 reserve/refund。保留行后与最终权威 F64 位型（x/y/vx/vy/mass）或已验证退役，包括最终槽代次和状态。容量满或身份缺失显式标不完整。
- `failure` 保留原 T381 首错、失败 attempt、回滚结果和 activeCommit。失败候选的暂存行不进入成功子步档。物理写路径、阈值与 endpoint wire 沿原合同。

[离线自测](units.jsonl)覆盖 9 项配对反例、两成功子步只呈现最后一笔、0.05 正授予且留余量、三个 ID 各一次全额退役，以及第二子步 prepare 注错保首段。注错前确认第二子步确有暂存 LAND。授权归档与无归档的权威检查点一致。使用实际生成的隔离模型，近床夹具在第 2 个回调完成带正授予退役的档。P013 配对自测使用受控渲染收据，真实交换由 T393 取证。T381 的 9 点回滚和 30 步 dry/wet 旧路差分通过。ArkTS/native Hvigor 构建及签名验证通过。

复跑：`node test/t392/generate.cjs <fresh-local-dir>`，再执行 `node test/t392/units.cjs <fresh-local-dir>/entry/src/main/ets`。
