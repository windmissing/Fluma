# T960 · G16 余时证书与集成 Demo 交接

2026-11-24 · Windows x64 Release。两项可独立复测。原业务 250 ms 和私有态封存条件继续生效。

## G16 · 两次真实事件后的余时

新版模块仅在两个原事件段都实际纳入真碰撞后尝试余时。冻结各盘位姿和速度，核两段与剩余时间的 FP32 账，随后全域检查四壁和盘对。静壁用二次最小值并扣舍入界。转壁先看含曲率项的全窗净空下界，模糊时有界细分。盘对用相对轨迹弦的最小距离，扣差加速度弯曲上界和舍入界。全部通过后才消费整个 R，再执行原累计位移、末端间隙和根收支审计。真实第三撞及未知区间保原 11/9。

`support` 独立开启原场景内的孤立定法向静壁承托。入口要求表示可证明的接触及精确零法速。使用单侧反力、静墙零功和原动量／角动量收支，通过其他盘壁全域扫掠才推进。实际耦合承托和转壁承托留待后续。精确接触的孤立标量夹具 `support-check` 分别验证静底、退壁和上推，给出实际局部力、冲量、分离和壁功。此证据处于独立局部坐标，与整场景的承托资格分别核验。

`third` 只由 fixture PE 启用。在 B13 static/J128 首根 micro 15 的两次真实解算之后，仅于私有候选替换盘 13 的位置与速度字。正臂核 wall 3，镜像核 wall 2，同检查约 4 µs 的向内夹根、未消费余时及原 BASE。任何该模式根都无状态晋升资格。

T960 模块和旧 T955 模块可用同一普通 PE 对照。Bundle 位于 `.codex/local-test/t960-g16-bundle`，含两种 PE、SPIR-V、种子及完整清单。

```powershell
$g = ".codex/local-test/t960-g16-bundle"
$seed = "$g/G16-B13.disk64"
$spv = "$g/g16_multi_v1.spv"
& "$g/t909_angle_probe.exe" g16-multi $spv $seed static 128 1 T960_B13_FREE diagnostics
& "$g/t909_angle_probe.exe" g16-multi $spv $seed static 128 1 T960_B13_SUPPORT diagnostics support
& "$g/t936_g16_freeze_fixture.exe" g16-multi $spv $seed static 128 1 T960_THIRD diagnostics third
& "$g/t936_g16_freeze_fixture.exe" g16-multi $spv $seed static 128 1 T960_THIRD_MIRROR diagnostics third mirror
& "$g/t936_g16_freeze_fixture.exe" g16-multi $spv $seed static 128 1 T960_SUPPORT_LOCAL support-check
```

每根有余时尝试时，冷档 `certified-tail-v1.u32le` 前 8 字为 header。word 1 为尝试数。接续 32 字一行，最多保留首 15 行及最后一行。word 1 分支码 1 是自由推进，2 是承托，3 是第三撞，4 是不确定或审计失败。words 2–7 为步初、步长、余时起点、R 和两段用时，8–11 为盘壁/盘对覆盖数及两类自由净空下界，12–13 为承托数和冲量。words 14–25 保存第三撞 ID、墙、TOI、余时、Q24 括号和签名界。words 26–30 给余时终点、实消费、首个承托 ID/墙和微步累计最大位移。Completion words 126–127 是尝试数与末状态。实际 GPU 结果按原 fence 与冷拷代关联。

## 单一 Demo 送验包

`.codex/local-test/t960-integrated-demo/Start Demo.cmd` 为同包唯一启动器。默认 PE 含 T942 的真实出生、已接受姿态与回正行为，使用原 T933 静态校验过的 35 派发生产模块与默认分频呈现。默认参数仅选择集成模式和双材料路由，无诊断旗标。同一脚本显式传入 `observe` 才开启 8 帧的真实 BIRTH 血缘观察，工件和生产节奏保持同版。

```powershell
$d = ".codex/local-test/t960-integrated-demo"
& "$d/Start Demo.cmd"
& "$d/Start Demo.cmd" observe
```

放行以同包采集中心/高位/实际已画 ±20° 的首粒像素、同粒向下段，加默认 4 秒见堆、60 秒后继续入料、松手留存和复位为准。T933 老包仍保存于 `.codex/local-test/t933-integrated-demo` 供差分。体验发布等同包验证。

## 构建与离线收据

| 工件 | SHA-256 |
| --- | --- |
| G16 module | `b9cab892f95da3b5402696d26ec41d1fe8acd2e89f636ada358208cf2c557923` |
| G16 普通 PE | `ee0cd23525eda1294bd0701c5b8a378cdd44ba4800f40d310e1a7a92e60a793c` |
| G16 夹具 PE | `1492f6b67262d8a513550495586f9a865845ef17590ae5352266935f03d1d2bf` |
| G16 Bundle manifest | `286f000b24dd2e56bea538c6daad9ce540e17ede09a9e43d0be9dfe47bbcb1f3` |
| Demo PE | `a81619efcdd17a80c31d21e5a443400f3adf2af2848507c0b6bd46bd84b61c1a` |
| Demo manifest | `abaa125ca1d6364411daaa8eb96aa458efc46b9440e33a745615ce44b057636e` |

Windows x64 Release 和 SPIR-V 校验通过。G16 完整展开源码为 48,916 B。T958 离线参考复核 52 个盘壁对、78 个盘对和 4 µs 负控。按新版 FP32 算法复算冻结 B13，下界约为 2.4747 nm 与 9.7900 µm。普通 PE 拒绝 fixture 注入，fixture PE 拒绝错种子。Demo 五件清单逐件哈希匹配，三个计算模块与 T933 逐字节相同。UHD 630 的新业务结果、承托、第三撞与同包体验交 tester/effector。
