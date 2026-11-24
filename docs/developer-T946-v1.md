# T946 · Windows Release 开发交接

2026-11-24 · Windows x64 Release。交付目标 6 并行库存候选、目标 9 原计划分段计时、目标 13 OP_48 三闭包及 F02 诊断、G16 接触修订候选。独立 checker 的固定样本和 12 个负控通过，GPU 原字、计时和业务接受由 tester/effector 接续。

## 目标 6

原 35、38 派发模块重编译 SHA 全等。新选项 `--m1-parallel-inventory` 在 PREPARE 和独立审计分别从本根各自状态提取槽、列质量和来源。64 线程并行提取，饱和列和等价于非负顺序饱和累加。首错、列顺序、宽总账与 ID 稳定去重由固定顺序输出。PREPARE 的角范围用与独立审计相同的已验证单调表做上下界搜索。

| 工件 | SPIR-V SHA 前缀 |
| --- | --- |
| 公用 SOURCE | e9c7c123 |
| 新 candidate | 90b33a3b |
| 新 commit 35 | f4f105b7 |
| 新 commit 38 | b8cd9b2a |

从仓库根用同一 PE 交错跑四臂。私有入口从历史参考保留原横坐标、请求量、物理 SOURCE Δ，使用紧域内公共 y=16。历史 y=64 在紧域上边界。两个 shader carry 字取同一参考，业务授权、fence、截止和退役使用各次实际 QPC。按档案核入参、许可和 SOURCE 前缀后再逐字比较，保留各次根身份及各自 35/38 计划字。此夹具覆盖空 BASE 启动的一次干沙 SOURCE → Q。

```powershell
$l = ".codex/local-test"
$pc = "$l/t946-ninja/hello_world_pc.exe"
python pc/vulkan/t933_off_reference.py test/reports/t927/runs/off-pair-t926-full/off-1.jsonl "$l/t946-m1-reference.bin"
foreach ($arm in @("base35","35","38","base38")) {
  $m = "$l/t946-m1-$arm"
  $profile = if ($arm.EndsWith("38")) { "--m1-audit-breakdown" } else { "--m1-integrated" }
  $opt = if ($arm -in @("35","38")) { @("--m1-parallel-inventory") } else { @() }
  & $pc --m1-window "$m/full_source.spv" "$m/quantum_candidate.spv" "$m/quantum_commit.spv" $profile @opt --m1-stage-timing --m1-private-off "$l/t946-m1-reference.bin" "$l/t946-m1-raw-$arm" OFF
}
```

重新生成模块时用 `pc/vulkan/build_t918_compact.py --assembled .codex/local-test/t280 --compiler <glslang> --out <目录>` 配相应的 `--integrated` 或 `--audit-breakdown`，优化臂加 `--parallel-inventory`。每包 `m1_static.json` 记录输入大小、布局及完整摘要。

## 目标 9

`--quantum-timestamps` 绑定真实逻辑序、queue、查询池和业务 fence 代，覆盖 upload、clear、六个 discovery、selected、authority、rejected prefix、replay、compare/pack/seal 及 tail copy。原业务拒绝继续生效。查询收据在真实 fence 退役后出具，晚读原件为私有诊断。原 SOURCE 路径可独立控制。保留 T945 指定的 `t913-split-v2` roleSet `f51229f4…`。

```powershell
$dm = "$l/t946-dm2-t913-split-v2"
$seed = "test/reports/t923/runs/t924-runs/raw-dm2-t913-split-v2-at-t921-steps1-r2/accepted/1663211005500200-1/FIXTURE128.bin"
& "$dm/pc_sand_dm2_host.exe" --modules "$dm/sand_assets" --source $seed --steps 0 --source-prepare-retired --raw-envelope "$l/t946-dm2-source"
& "$dm/pc_sand_dm2_host.exe" --modules "$dm/sand_assets" --source $seed --steps 1 --source-prepare-retired --raw-envelope "$l/t946-dm2-q-control"
& "$dm/pc_sand_dm2_host.exe" --modules "$dm/sand_assets" --source $seed --steps 1 --source-prepare-retired --raw-envelope "$l/t946-dm2-q-timing" --quantum-timestamps
```

裁槽待续。T923 同进程的业务 Q 为 `VkResult=2`、`tailRead=false`，无同 fence 分段；2404 逻辑槽及 EVENT/PATH 属晚尾。静态核查确认当前 shader 在微步空体检查前仍逐槽校验和递增 control[16]。直接跳过会改变原顺序证明与 seal。由 effector/T948 采同 fence 分段及真 latch 后，选择确有收益且可保计数的单个 extent，再交 developer 实施同代优化和对照。

## 目标 13

同版 PE 可资格验证旧 `OP_48` 及新 `OP48_VISCOSITY`（812db31f）、`OP49_PRESSURE`（ca603a59）、`OP48_REMAINDER`（64101778）。四个模块摘要逐字匹配 T945；原 OP66、布局、820 槽及 OP7/OP17 分路保留。F02 选项在旧分段上追加 818 audit 和 ACTUAL/REPLAY 的 J1 micro 0 压力 cycle 0、黏性 block 0 边沿。按现有槽快照确认实际激活后解释标记。F02 保持 `PRIVATE_PROJECT` 身份。

```powershell
foreach ($role in @("OP_48","OP48_VISCOSITY","OP49_PRESSURE","OP48_REMAINDER")) {
  python pc/vulkan/qualify_role.py $pc "$l/t946-qual-$role" $role --stack-dumps
}
$fixture = "test/reports/t868/fixtures"
& $pc --g13-candidate "$l/t946-f02-original" --g13-fixture F02 "$fixture/F02-config128.bin" "$fixture/F02-state.bin" --g13-diagnostics hot --g13-tail-timing --g13-slot-snapshot --g13-compile-trace
& $pc --g13-candidate "$l/t946-f02-split" --g13-fixture F02 "$fixture/F02-config128.bin" "$fixture/F02-state.bin" --g13-diagnostics hot --g13-tail-timing --g13-slot-snapshot --g13-split-op48 --g13-compile-trace
```

正式 ABBA 比较两臂统一 snapshot 设置。T909 seal 对照在同一角色选择上单独切 `--g13-seal-v1`。

## G16

模块 SHA `792f5986328b796aa4bea47b601899cce55aef83e5f0e76ed3d29afe7b718cda`，展开源码 37,630 B。新 Release PE 也接收 T936 原模块作对照。壁间隙局部求值保留短段位移，区间推进先排除较早向内根，外扩 Q24 后再验两端符号和 50 µs 宽度。求解仅纳入同时真碰撞、接触承托及有剩余穿透条件的预测，按 admitted CSR 行和迭代，维持原 16 轮、残差、守恒与拒绝原子门。原轴壁及转壁区间需要 tester 对冻结输入独立复算。

真碰撞、承托、预测计数分别为 Work `witness+27..29`。壁夹根首错的 `witness+8..15` 依次为两端 gap/error、两端角、起始法向速度及曲率界。原 journal 及首错字段继续可读。

```powershell
python test/t928/fixtures.py --out "$l/t946-g16-seeds"
$g16 = "$l/t946-g16-release/t909_angle_probe.exe"
$spv = "$l/t946-g16-shaders/g16_multi_v1.spv"
& $g16 g16-multi $spv "$l/t946-g16-seeds/G16-B13.disk64" static 128 2 "$l/t946-g16-b13-static"
& $g16 g16-multi $spv "$l/t946-g16-seeds/G16-B52.disk64" fast 256 480 "$l/t946-g16-b52-fast" present
```

主 PE、DM2 split-v2、G16 探针及 FREEZE 夹具均使用 MSVC/Ninja Release。生成器和发布命令沿 T933/T924/T936，诊断工件留在上述本地目录。编译和独立 checker 固定样本覆盖完成。目标 9 裁槽、目标 6 多库存档同输入、目标 13 GPU 资格及 G16 冻结八臂为后续实证项。

| Windows Release | PE SHA-256 |
| --- | --- |
| `hello_world_pc.exe` | `bc056740ea6238ff7e4ff22b3a90fe1c0f95b410024ca59bae4cb050cf0e3a9b` |
| `pc_sand_dm2_host.exe` split-v2 | `c25db32d210ab86dfe12809028d3617726b9dc8517e095ef1fdf853556f5a48d` |
| `t909_angle_probe.exe` | `1299d3fddf0f585da86d1b45473f987c21f995991fd12dbc992063b686f1eb19` |
| `t936_g16_freeze_fixture.exe` | `b1aa28d814d953ce40f53aab8aa0fc185132813cc4260a04817330b8f9ef8334` |
