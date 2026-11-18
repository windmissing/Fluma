# T496 v5 · CREATE_ONLY 三臂 900s 有界执行（T518/T547 合同，同机同环境）

日期：2026-11-18，tester，不使用真机（host GL 离线臂）。
被测：t496.exe（t492-m1-delivery 冻结源，CREATE_ONLY 三臂分支）。
证据：`.codex/local-test/t496/t496_log_t496v4c-{c0,c1,c2}.ndjson` + 各臂
`{C0,C1,C2}-files/gpu-program-cache/v1/`。

## 三臂结果（每臂独立新进程+新输出目录+唯一 attemptId，900s wall 界）

| 臂 | 暖种子 | Store.Configure | 冷/暖 bin 产出 | 卡点 |
|---|---|---|---|---|
| C0 | EMPTY_CACHE | ✓ entries=0 | **冷编译 3 bin** ✓ | CREATE_WAIT iter=45000 |
| C1 | SEEDED_COMPUTE3 | ✓ entries=3 | 载入 3 compute bin ✓ | CREATE_WAIT iter=45000（同 C0） |
| C2 | SEEDED_ALL6 | ✓ entries=6 | 载入 6 bin ✓ | CREATE_WAIT iter=45000（同 C0） |

- 三臂 **STORE_CONFIGURED PASS**；每臂 cache 原子产出与 T518 §3 预置一致
  （C0 冷编译、C1/C2 暖载入均成功）
- 三臂到 **900s wall 界**统一停在 CREATE_WAIT（iter 计数 0→15000→30000→45000，
  ~2ms/iter sleep 下界与 T518 观察一致），**无首 ENTER 无 EXIT 的 GL 调用悬空**
  （gltrace 无悬空 ENTER 记录——本轮 sink 落盘路径仅覆盖 D1 轨，CREATE_ONLY 轨
  ENTER/EXIT 明细未注入，如实记录）
- **arm 间差异核**：暖 compute bin（C1）与全部 6 bin（C2）均不改变卡点 →
  按 T518 §3 判据：**"所有 arm 都能到资源 commit 而不能 READY = 归测试恢复握手"**
  （资源已返回但 WAIT_NEW_SESSION/RecoverySessionLocked/首 pulse 握手未前进），
  非 driver/JIT 死锁、非编译/载入缺陷

## T518 §3 判据对照

| 判据 | 结果 |
|---|---|
| C1 过 compute 卡 scene → 范围缩 scene | 不适用（C1 也未过 compute 段后的握手） |
| C2 BINARY_LOAD 仍卡 → 不再重复归因 compile | ✓ 确认（compile/BINARY_LOAD 全通） |
| 所有 arm 到资源 commit 而 D1 不能 READY → 归测试恢复握手 | ✓ **本轮直接命中** |

## 修正与边界

- v4a/v4b 探针失败根因：exe 需 CokePLUS DLL（0xC0000135 STATUS_DLL_NOT_FOUND）+
  attempt 文件复用保护（rc=5）——修正为 PATH 注入+唯一 attemptId
- T518 §2 表"暖种子根本未接入"已在本轮验证修复（Store.Configure 三臂全 PASS 且
  entries 与臂配置一致）
- CREATE_ONLY 轨 GL ENTER/EXIT 明细 sink 未注入（本轮 gltrace 无悬空对）——按 T518
  §3.5 记录为接口缺口：需要 create-only 路径的转发层接线（回 architect 确认函数/
  接口清单），不猜测
- D1/D2 完整臂、非空 SOURCE 正控：NOT_RUN（CREATE_ONLY 未过握手，依 T518 顺序要求）

## 判定

- 三臂 900s 有界执行完成，证据齐全；**卡点收窄至"资源 commit→恢复握手"**（非编译/
  非载入/非 driver 死锁）——回 architect 按 T518 §2"READY 交接断层"行继续归因
  （tester 侧修复项：RecoverySessionLocked/首 PagePulse 生产交接接线）
- 空账保持 NOT_MEASURED；不冒 D1/D2/32cycle 通过
