# T946 · 续卡开发交接

2026-11-24 · Windows x64 Release。沿 [T949 实测](../test/reports/t949/report.md) 交付 DM2 单一 REPLAY extent 裁剪、G13 三闭包同版、集成 Demo 血缘观察和目标 6 四臂发布包。

## 目标 9 · 独立 REPLAY 尾部

T949 的同 fence 分段中 REPLAY 为 431.708 ms，迟到 Completion 的真实选中 J=1。新 profile `t946-replay-indirect` 只覆盖独立 REPLAY 的逻辑槽 2013–2396。槽 2007 按同根 GPU latch 生成 32 个 dispatch 参数。每个微步的原 12 个命令仍按原逻辑 ordinal 提交，超实际 J 的工作组数为零。槽 2397 校验整个参数表、同一 latch 和 `control[16] == 2013+12*J` 后，将原逻辑计数补齐到 2397，再进入原计数校验与物理审计。非法 latch 或计数走原错误门。J=0 保留全排程，真实拒绝前缀保留。

SOURCE、DISCOVERY、AUTH、拒绝前缀、REPLAY 的活跃微步和读取 BASE 的绑定均沿原计划。旧控制包继续走 2404 个直接 dispatch。新版也编码 2404 个 Vulkan dispatch 命令，另有一道参数可见性 barrier。以 J=1 为例，其中 372 个 REPLAY 间接命令发出零工作组。收益须以同 fence 实测计量。冷档补 `replayIndirectArgsValid`、`replayZeroWorkgroups`、`encodedDispatches` 和 `extraIndirectBarriers`，完整参数表位于 WORK 原布局末端 1,311,360 B 起的 388 B。

| 冻结项 | 收据 |
| --- | --- |
| 控制 / 优化 roleSet | `f51229f4…` / `4bf72431…` |
| 模块离线比对 | 仅 opcode 33 与 39 变化。SOURCE、逻辑表和 push/binding 布局摘要一致 |
| J=0/1/2/4/8/16/32 | 零工作组命令分别为 0/372/360/336/288/192/0 |

从仓库根执行。各臂使用新输出目录。

```powershell
$p = ".codex/local-test/t946-resume-package"
$seed = "test/reports/t923/runs/t924-runs/raw-dm2-t913-split-v2-at-t921-steps1-r2/accepted/1663211005500200-1/FIXTURE128.bin"
foreach ($arm in @("control","replay-indirect")) {
  $dm = "$p/dm2/$arm"
  & "$dm/pc_sand_dm2_host.exe" --modules "$dm/modules" --source $seed --steps 1 --source-prepare-retired --quantum-timestamps --raw-envelope "$p/raw-$arm"
}
```

同窗优先复测真实 J1，另保 J2/J4 和 J=0 负控。分别核业务接受、真实 fence 退役、冷诊断，逐字检查原物理证书、拒绝前缀与独立 REPLAY。

## 目标 13 · 三闭包同版

发布包将 PE 和原 OP_48、黏性、压力、其余三闭包的 SPIR-V 与清单一并封存。三闭包继续使用 T949 实际创建成功的同一字节版本 `812db31f…/ca603a59…/64101778…`。修正独立资格监督脚本的角色名校验，四臂可使用同一监督入口。F02 仍可记录同 fence 的 ACTUAL/REPLAY、J1 首个黏性块/压力周期、815–819、tail copy 和槽快照。

```powershell
python pc/vulkan/qualify_role.py "$p/hello_world_pc.exe" "$p/qualify-g13" OP_48 OP48_VISCOSITY OP49_PRESSURE OP48_REMAINDER --stack-dumps
$fixture = "test/reports/t868/fixtures"
& "$p/hello_world_pc.exe" --g13-candidate "$p/f02-split" --g13-fixture F02 "$fixture/F02-config128.bin" "$fixture/F02-state.bin" --g13-diagnostics hot --g13-tail-timing --g13-slot-snapshot --g13-split-op48 --g13-compile-trace
```

T949 的 F02 业务根仍超期，求解优化接 T951。

## 集成 Demo · 出生与同粒观察

`--m1-observe-birth FRAMES PREFIX` 从真实按住产生的首个接受干沙 SOURCE 取有效前缀，最多检查四条 GPU 原始事件。以 BIRTH 的 id、slot generation、sourceId、origin 与 SOURCE nonce 建立观察锚。观察最多 12 个业务呈现，使用同 Q 的实际 publication 样本，取指针、指针下方和跟踪粒子的三个 fenced ROI，带同帧角度、根 nonce、出版代、graphics fence、图像索引及像素 SHA。BIRTH 原字与 SOURCE 命令随日志导出。每个观察帧继续用原冷档封存真 Q；只按该 Q 的事件后缀和同粒身份标出干 MOVE 或 MOTION_CONTACT。取不到出版样本或沙色像素时分别报告 `NO_MATCHING_ROOT` 和 `NO_BIRTH_PIXELS`。维持默认呈现节奏和 SOURCE/重力规则，记录实际 `pacedPresent`。沙色计数与同粒像素误差由独立 checker 复核。

```powershell
$m = "$p/m1/35"
& "$p/hello_world_pc.exe" --m1-window "$m/full_source.spv" "$m/quantum_candidate.spv" "$m/quantum_commit.spv" --m1-compact-demo --m1-integrated --m1-dual-source --m1-parallel-inventory --m1-observe-birth 8 "$p/birth-center"
```

以不同前缀重复中心、高位、实际画出 ±20° 后的下按。物理和渲染观察仅随显式开关启用。

## 目标 6 · 四臂与工件

包内 `m1/base35`、`m1/35`、`m1/base38`、`m1/38` 对应 T949 的四臂。私有同输入命令沿 [首轮开发交接](developer-T946-v1.md) 替换 PE 和模块目录即可。T949 四臂 admitted 126053，SOURCE ledger 逐字节一致。持续推进、帧率及代表水/沙负载留实测。既有 G16 工件沿首轮交接，新的两个门等 T951。G15 干前沿等 T952。

Release 包位于 `.codex/local-test/t946-resume-package`，122 件的 `manifest.json` SHA-256 为 `482d5f4fc259b82ec2f6e14aface06931ce98e86c6f17abf5e8bc52feb70ce00`。主 PE `d9e6f89c…`、DM2 控制 `67c6d7b1…`、DM2 间接 `dd55cb8c…` 均编译成功。SPIR-V 验证、两套逻辑表及 SOURCE 模块比对、T948 离线自检和观察参数拒绝检查通过。UHD 630 复测由 effector 承接。
