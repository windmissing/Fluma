# T897 · T895 原件与 T894 B 独立判定

Windows PC 离线 · tester · 2026-11-23。复核 T895 提交 6389bec8 的四份 JSONL 与一份 M1PROOT1。机器收据见 [offline.json](offline.json)。

| 范围 | 已核实 | 仍缺证或未到达 |
| --- | --- | --- |
| 空箱 | 两次均为 null root、零库存与零提交。真实初始化 fence 后同代空帧入队，随后出现 pointer READY。 | 更换几何代后的旧输入拒绝和空箱重建。 |
| 重力 | +15°、±30°输入接收，两条超 ±30° 拒绝。负 30° 输入序号 4 及原 QPC 和三份同根 Completion 一致，接受角字为 −7679/256°。 | 完整跨代重力序列；+15°与+30°仅到输入接收。两份原件均有后续 M1_QUANTUM_CONTINUATION_FAILED。 |
| 同根原档及像素 | 根 15944411374190 的 63048 B 原档通过 T496 容器、摘要、Completion、bank/publication/event backing 核对。768 B 上屏读回与已排队 frame 的 slot、代、image index 和出档记录吻合，像素 SHA 一致。 | 床与粒子共用 [64,380,8,8]，两层缺独立像素证据。该根 bedEventCount=0，逐笔 LAND 未到达。 |
| G13 | full/hot 各 16 个成功创建并执行 OP66 的角色。两臂 QUANTUM 都先触发 GRANT_DEADLINE，原退役等待 vk=2，随后同 nonce/device/queue/fence/generation/query pool 成功排空及确认，18 个时间戳可用。复算 GPU 根跨度约 1699.335/1523.317 ms。 | 两臂 acceptedRoot 均为空，可信流场与最终提交未到达。 |

T894 B checker 直接读 `M1_D1_ACTION`、`M1_D1_PULSE_OWNER`、`M1_D1_OWNER_COVERAGE_V1`。按 run/geometry/session/seq 和 actionId 排重，逐条核真实 callback/receipt/owner QPC，重算每会话 queued/terminal masks、接受/拒绝/撤销数及 intake 水位。比较两次 OFF 实际动作距 READY 的间隔、两条原生时标、owner 延迟和配额状态，以及 T496 解出的 SOURCE 原量与原间隔。冻结脚本时标只用于动作表核对。运行 `python test/t897/check.py --off <OFF1.jsonl> <OFF2.jsonl>`。此命令输出首个时域或输入差异，正式 OFF×2 由 T496 在原件到件后接退役、根与原字严格 join。

运行 `python test/t897/check.py` 复算原件和合成 B 测试。合成两代共 48 拍，覆盖重锚撤销，另有 7 项定向负控以及同输入、首个时域差及首个 SOURCE 差检查。T897 未执行设备测试，当前无新版 OFF×2 原件。
