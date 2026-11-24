# T936 · G16 活动槽 flags 修复

G16-B 的 FREEZE 对紧凑活动前缀逐槽要求 Disk64 第 13 字完整等于 `1u`，第 14、15 字继续要求为零。保持原检查顺序、首盘号、种子全字节 SHA、容量、250 ms、同 fence/代、守恒与私有提交门。普通 `g16-multi` 仍只接受两份 T928 种子和本版模块。

新版完整 shader 输入 32,044 B，SHA-256 `db6efc510cbdb482b6967da09bb66fe45f539a4fbf0fc65bf1c9744e84121124`。SPIR-V 为 156,140 B，SHA-256 `786bb1db9b8d3a679042bdc36d014226d68647913770d503192c297e0c10e361`，与 T934 试编逐字一致。Vulkan 1.1 SPIR-V 静态验证、绑定 0–3 反射及完整源码尺寸门通过。Windows x64 Release 的成对工件位于 `.codex/local-test/t936-g16-bundle`，文件哈希和编译收据见该目录的 `manifest.json`。

从 MSVC x64 命令环境重建。`$compiler` 为本机 glslang，`$vk` 为 Vulkan 头目录。

```powershell
$local = '.codex/local-test'
python pc/vulkan/build_t909_angle_probes.py --compiler $compiler --out "$local/t936-g16-shaders"
python test/t928/fixtures.py --out "$local/t936-g16-seeds"
cmake -S pc/vulkan/sand -B "$local/t936-g16-release" -G Ninja -DCMAKE_BUILD_TYPE=Release -DSAND_FAMILY=dm2 -DPC_G16_FREEZE_FIXTURE=ON "-DG13_GLSLANG=$compiler" "-DVULKAN_HEADERS=$vk"
cmake --build "$local/t936-g16-release" --config Release --target t909_angle_probe t936_g16_freeze_fixture
```

## 负控与交接

`t936_g16_freeze_fixture.exe` 由独立构建目标生成。它先校验原始种子与模块，指定根开始时另分配一个同范围的状态 SSBO 副本，只在副本指定盘的一个字写入坏值。冷读回包括原始 BASE、修改后的 BASE 和测试候选的完整 64 KiB bank，记录每份 SHA-256、改写位置及实际首盘。该根永远保留原私有 commit、state/scene generation 与场景槽。预期 GPU 收据是 outcome/reason/stage 依次 `2/2/1`，`stageMask=0x801`，无 PACK 和物理推进。只有预期拒绝、同 fence 及冷原字都成立，夹具才返回 0。

从仓库根运行，输出目录使用新路径。常规入口的两个根让 tester 比较换 bank 后的 flags 和保留字。夹具以 `ROOTS=2` 选第二根，前一根须实际接受后才注入故障，故障之前若有物理拒绝则保留原首错。

```powershell
$p = '.codex/local-test/t936-g16-bundle'
$s = "$p/g16_multi_v1.spv"
$seed13 = "$p/G16-B13.disk64"
$seed52 = "$p/G16-B52.disk64"
& "$p/t909_angle_probe.exe" g16-multi $s $seed13 static 128 2 g16-b13-two-roots
& "$p/t909_angle_probe.exe" g16-multi $s $seed52 fast 256 480 g16-b52-fast present
& "$p/t936_g16_freeze_fixture.exe" g16-multi $s $seed13 static 128 1 g16-bad-first --freeze-fault 1 flags 0
& "$p/t936_g16_freeze_fixture.exe" g16-multi $s $seed13 static 128 2 g16-bad-second-bank --freeze-fault 13 flags 3
& "$p/t936_g16_freeze_fixture.exe" g16-multi $s $seed52 static 256 1 g16-bad-late-slot --freeze-fault 52 reserved1 1
```

`--freeze-fault SLOT FIELD VALUE` 可选 flags 的 0/2/3、reserved0/reserved1 的非零，以及 generation/origin/material 的错误值。正常 PE 用变造种子验证 `G16_SEED_SHA`，两版 PE/SPIR-V 交叉组合验证 `G16_MODULE_SHA`。B13/B52、J128/256、快慢速、真实窗与首个新拒绝由 PC 复测覆盖。

开发已复核两组种子每个活动槽的末三字为 `[1,0,0]`。调用 Vulkan 之前的 19 项负控通过，含双向混配、两种种子的 10 个变造样本、正常 PE 拒绝注入参数及夹具参数错项。T930 离线自检通过。GPU 注入、换 bank 及连续场景由 tester/effector 执行。
