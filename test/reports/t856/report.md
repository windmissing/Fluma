# T856 · PC 描述符修订后交互首帧与冷档复测 · tester 视角记录（effector 执行）

2026-11-23 · effector · 真实 Windows PC（Intel UHD Graphics 630，Vulkan 1.2 core）。复用 T853/T855/T836 工件，不等待其他卡形式关闭。

## 1. 工件核验（与 T853 handoff 一致）

| 项 | 值 | 校验 |
|---|---|---|
| PE | `.codex/local-test/t853-final.exe` | SHA256 `9bdbcf7d…4e15e`，与 build.json exeSHA 一致 |
| full_source.spv | `1912af9e…c7162` | 一致 |
| quantum_candidate.spv | `b8f0e327…8c85c` | 一致 |
| quantum_commit.spv | `caf0614c…906ce` | 一致 |
| 角表 | 245776 B | 上传字节与 JSONL m1ColdAngleUploadBytes 一致 |

## 2. 固定 SOURCE 复验（`test/reports/t856/fixed.jsonl`）

`--once --boot-only --probe-m1-source`：24 行，终态 `M1_DRY_SOURCE_ACCEPTED`；描述符 16/16 非空、16 组 update、ownerCode=0、queue/fence 均 0、m1AdmittedQ=131072——与 T853 build.json 的 fixed 基线（131072）一致。

## 3. 交互首帧（run2 / run3，真实 Down/held/Up）

驱动方式：Win32 `PostMessageW` 直发 WM_LBUTTONDOWN/MOVE/WM_LBUTTONUP（SendInput 因后台窗口不收输入，见 §6）。

| 项 | run2（短按） | run3（25 s 长按+移动） |
|---|---|---|
| 行数 / 指针事件 | 35 / DOWN→UP | 141 / 109 |
| 角表 fence | 0（245776 B） | 0（245776 B） |
| 描述符 | 16/16 非空、16 updates | 同左 |
| SOURCE owner/fence | 0 / 0 | 0 / 0 |
| QUANTUM owner/fence | 0 / 0 | 0 / 0 |
| m1AdmittedQ | 126172 | 136485 |
| M1_QUANTUM_PRESENT_QUEUED | ✔ | ✔ |
| M1_QUANTUM_PRESENT_RETIRED | ✔（wsiResult=0，publicationReleased=true） | ✔ |
| 同根 shader Present | windowFrame.acceptedRoot == m1QuantumNonce（同根链 run/config/sourceCommit/physicsCommit/generation 全对齐） | ✔ |

Up 后退役链完整：`M1_QUANTUM_PRESENT_RETIRED` 终态、发布释放、DeviceWaitIdle 0。run1（带 MOVE 拖动）按设计被时钟判 `CLOCK_GAP`→`PERMIT_REVOKED`（拖动中 anchor 断档），如实保留为负证。

## 4. 冷档（M1PCOLD1，同进程，交 T848）

- run2 档 `test/reports/t856/m1pcold1-run2.bin`（93872 B，payloadSHA `3a358054…19eaf`）
- run3 档 `test/reports/t856/m1pcold1-run3.bin`（94384 B，payloadSHA `dbc5fc34…6450`）
- 同进程 JSONL：`run2.jsonl` / `run3.jsonl`（m1ColdPayloadSHA/m1ColdBytes 与档案一致）

### 4.1 发现一：冷档容器写错 24 字节（developer 修）

实测文件布局 `magic(8)+sizes(32)+digest[前8字节]+raw`。原因在 `m1_quantum.inc` CaptureM1Quantum：

```cpp
write(digest.data(), DWORD(digest.size()))   // std::array<uint32_t,8>::size()=8（元素数）
```

`.size()` 是元素数 8，实际需 32 字节，导致 SHA-256 摘要只落盘前 8 字节。`m1ColdBytes` 字段同样按 8 计（93872=93824+8+32+8）。测试侧无法修生产码；已生成容器修复副本（载荷逐字节不变，仅补全 32 字节摘要头）：
`m1pcold1-run2-normalized.bin`（93896 B）、`m1pcold1-run3-normalized.bin`（94408 B），并验证 `sha256(raw)` 与 JSONL m1ColdPayloadSHA 完全一致。

### 4.2 发现二：T848 join 判据 ROOT_LINEAGE 与现行采集设计矛盾（architect 裁定）

`test/t848/check.py` 要求 `pair(q,24)==pair(s,24)`（QUANTUM 与 SOURCE 同帧）。实测档内 source frame=1、quantum frame=2（eventStart=0）。根因：首帧 `clock.Claim` 时 pending=0（首个脉冲只置 baseline，不产配额），QUANTUM 走新帧路径；而 `--m1-cold` 又单帧呈现（nextPublication 被禁用），单次采集永远拿不到同帧对。两份归档在 `TRUNCATED_PAYLOAD`（修复后）→`ROOT_LINEAGE`（本条）处失败，其余判据（SEAL/事件长度/nonce/命令字/呈现身份）全过。该矛盾超出 effector 权限，交 architect 裁定 T848 判据或采集时序。

## 5. 结论

T856 卡面范围全部复验通过：固定 SOURCE、角表 fence、16 非空描述符至 POINTER_READY、真实 Down/held/Up、SOURCE→QUANTUM→同根 Present、退役、同进程 M1PCOLD1+JSONL 已交。两项新发现（冷档摘要写短、T848 同帧判据矛盾）按角色归 developer / architect。GPU 驻留与 shader 管线全程使用，无 Canvas 回退。

## 6. 附：交互注入方法备注

- SendInput 对非前台窗口不产生 WM_* 输入（本次实测），后台驱动改用 PostMessageW 直发窗口消息，DOWN/UP 均被 `Input()` 正常接收并进入 hold 语义。
- 首帧必须"按下后不动"：wait 循环要求同 hold 下两次连续脉冲且间隔 ≤ GapNs(33.3 ms)；MOVE 会破坏 held 或引入 anchor 断档（run1 负证）。
