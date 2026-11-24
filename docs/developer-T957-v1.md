# T957 · G14 解析切格 Windows Release 交接

2026-11-24 · 独立 Vulkan 原型。交付固定周期平床 A、双格开面 B、移动斜床 C 两档。GPU 执行切格、开口与通量、三分项牵引、连通分量审计与私有封存。CPU 装载冻结夹具、核同 fence/代并导出 LE 原字。沿 [T956 ABI](architecture-T956-abi-v1.json) 保留四个 SSBO、64 lanes 和最多 26 派发。证据身份为 `ANALYTIC` 或 `MANUFACTURED`，正式水场输入由 T611 接可信 GridView。

## 工件与运行

Release 位于 `.codex/local-test/t957-dm2-baseline`。编译目标为 `pc_g14_boundary_probe`，同目录 `g14_assets` 提供三角色 SPIR-V、展开源、原夹具、布局头和 `modules.json`。宿主对 manifest、模块和夹具均校验冻结 SHA。完整 GLSL 每角色为 23,113 B，低于 49,152 B。

| 工件 | SHA-256 |
| --- | --- |
| PE | `8168be2dde084ad34cc8fed59ab68ca14134165d8661b2da604455e27452e505` |
| modules.json | `51cb1aaf2483f8e4975850a12e2805f857900267586194b368b7ff64c56a7a7f` |
| geometry.spv | `f002ef066b8efdb398b9490f51f7ac67a694fd1246b7802712ea154f8375095f` |
| transport.spv | `e4d4910732614bfe709abad907328d877d47fa71d9eaea656badb9dce7342696` |
| audit.spv | `7c3589b5b10486c97070421c4e013138c9b40f234bd47fd2004e2bd5407cd031` |

```powershell
$r = ".codex/local-test/t957-dm2-baseline"
$a = "$r/g14_assets"
foreach ($fixture in @("A","A-reverse","A-gauge","B","C-20x10","C-40x20")) {
  & "$r/pc_g14_boundary_probe.exe" g14-boundary "$a/modules.json" "$a/$fixture.g14" ".codex/local-test/t957-run-$fixture"
}
foreach ($fixture in @("A-bad-gauge","B-closed","B-closed-flux","B-wrong-sign","B-wrong-time","B-wrong-generation")) {
  & "$r/pc_g14_boundary_probe.exe" g14-boundary "$a/modules.json" "$a/$fixture.g14" ".codex/local-test/t957-run-$fixture"
}
```

使用现有 sand CMake 工程，配置 `SAND_FAMILY=dm2`、`SAND_ROLE_VARIANT=baseline`、Vulkan headers 和带 SPIR-V 验证的 glslang。采用 T956 允许的 Float64 路径，创建设备时核 `shaderFloat64`，每根 GPU FREEZE 核 `1+2^-30` 拆分的两个实际 FP32 原字。PC 复测须实录该能力门。

## 证据读取

- `roots.jsonl` 保存 nonce、根和几何代、输入范围、lease、同一真实提交 fence、业务及退役结果和各文件 SHA。及时完整私有封存退出码为 0，业务拒绝为 1，装载或能力门报错为 2。超期结果仅在原 fence 确认真退役后读回为 `late-*.u32le`，日志中一直保留 `acceptedRoot:null`。
- `input-N.u32le` 含实际上传的 4,096 B、根代、nonce、精确有理时刻和解析参数。参数区起点 2,304 B，八个 LE binary64 依次为 L、H、ρ、μ、P0、物理压力参考、规约平移量和势差。解析场是由冻结参数重建的表达式，真实规则网格 MAC 原字由后续 accepted G13 provider 交接。
- `root-N.u32le` 前 256 B 为 Completion。其后每个名义子区间占 410,880 B，按 ABI 放几何与审计。Completion words 16–24 保存首错及 measured/expected Real64，32–35 分别标记实际区间封存 `c0149571` 或该子区间的拒绝快照 `d0149571`。不带这两类标记的区间排除出有效物理数据。
- Flux32 word 6 的高两组 2 位为起末符号，1 为正、2 为负、0 为零。低 25 位在有符号转折时给相对于子区间的 Q24 位置。冻结 C 的开面按符号分段积分，积分节点出现未隔离的异号时报告 reason 11。
- 积分区 header words 2、3 是两侧实际载荷记账数。8–31 和 32–55 分别为沙侧与水侧的 12 项 Real64 总量。次序为压力、法向黏性、切向黏性的二维冲量，再接三个角冲量和三个壁功。C 首根首区间的 56–61 记录 t=0 沙侧合力 x/y 与水侧壁功率，word 62 为有效标。

## 已核边界

C 的精确时刻与 T956 表逐项一致，粗档 25 根合计 30 段，细档 50 根合计 73 段。独立 Python 参考以端点切面积和 12 点时间积分复算，两档最大逐格余额分别约 `3.56e−20`、`1.74e−20 m²`。这是离线参考结果。Windows x64 Release、SPIR-V 验证、T952 静态基准、T953 自检及运行前资产误配拒绝均通过。Vulkan 原档、GPU 数值门及真实耗时由后续 PC 验证提供。
