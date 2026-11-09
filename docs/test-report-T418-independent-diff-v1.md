# T418 · 单沙最终包采集接口独立差分

2026-11-09，tester。不使用真机、不改产品。消费 T415 oracle 与 T414 最终实现（`test/t413/` controller.inc/T413Capture.ets/capture_hold.py/clock.py），独立做状态与时间包络差分。产出供 T408 真机续测消费，不作为开工门。

## 1. 交付物

| 文件 | 用途 |
|---|---|
| `test/t418/capture_state_diff.cjs` | 状态差分执行器：对隔离生成工程（`test/t413/generate.cjs` → `.codex/local-test/t418-diff`）的 `render/T413Capture.ets` 逐场景实际执行，8 场景输出证据 |
| `test/t418/diff_units.py` | 主判：T414 实测输出 × T415 oracle 语义逐条对照 + 时间包络双实现同输入比对 + 复核链静态对照 |
| `test/reports/t418-capture-raw.json` | 8 场景原始执行证据 |
| `test/reports/t418/t418_diff_verdicts.json` | 33 条差分判定（33 AGREE / 0 DIVERGE） |

复跑：`node test/t413/generate.cjs .codex/local-test/t418-diff` → `node test/t418/capture_state_diff.cjs .codex/local-test/t418-diff` → `python -X utf8 test/t418/diff_units.py`。

## 2. 差分结论（33/33 无语义分歧）

| 任务书覆盖 | 差分条目 | 结论 |
|---|---|---|
| 失败预约 | OPEN 中新预约 `HOLD_ACTIVE` + 状态/request/phase/actionSerial 零副作用 | 一致（T413Capture.reserve 不自查 state，由 controller.reserveCapture 的 canReserve 先拒保证——差分按 controller 合同路径构造） |
| 旧 OPEN/ACK | 三重拒绝（attempt/request/空 SHA）+ archive 保持 PENDING；迟到的旧 attempt ack 不清新 evidence | 一致（reason 命名不同但语义同：`ARCHIVE_IDENTITY`↔ATTEMPT/REQUEST_MISMATCH、`ARCHIVE_NOT_READY`↔EMPTY_SHA，记 NOTE） |
| 同 id seal | 重复 seal 同回执 SEALED、terminal 文本不可变、endMonoMs 冻结、错 id `HOLD_IDENTITY` 拒绝 | 一致 |
| PNG SHA | verify_bundle 绑定 fileBytes+fileSha256（64 位强校验）在时间门之前 | 一致 |
| 越 hold 负控 | 同输入双实现（T414 clock.envelope 精确有理数包络 vs T415 oracle time_gate）：正例 PASS、先 seal/包络越界→`CAPTURE_INTERVAL_OUTSIDE_HOLD`、<3 样本/回拨→`CLOCK_MAPPING_UNKNOWN` | 一致 |
| 生命周期/到期/取消/释放 | EXPIRED 只失效不造成功 seal；重复 cancel 保留首因（`KEYBOARD`）；SEALED 未归档/未 savedTerminal 不能 release；geometry 变化 INVALID 首因 | 一致 |

## 3. 发现并修复：T415 oracle A15 断言笔误

差分过程发现 T415 `hold_oracle.py --selftest` A 部分实际 18 ok / 1 bad（提交 be06143 即如此，非工作区改动）：A15 断言期望首因 `INVALIDATED:keyboard`，但 cancel 路径实现（正确地）产生 `CANCELLED:<reason>`——`INVALIDATED:` 前缀属于 geometry/hide 失效路径。语义（重复取消保留首因）两版实现均正确，属断言字符串笔误。已修正断言并注明勘误来源，修正后 T415 selftest 19+8 全过（0 bad）。T415 报告中"19/19 PASS"的声明在断言修正后恢复为真。

## 4. 基线与边界

- 差分前提基线全绿：T414 units.py 16/0、runner_units.py 4/0、controller_units.cjs 11/0（对隔离工程；工作区无 T394Phase.ets 属预期，工具按设计对生成工程运行）、T415 selftest 27/0（勘误后）。
- release 后 `terminal()` 返回空串（RELEASED 不在 terminal 分支）为 T414 产品语义，差分器按 release 前后分别断言，非缺陷。
- 本差分为离线合成层；真机截图可见性、真实时钟量化与系统截图行为由 T408 按 `test/t413/README.md` 记录。无阻塞。
