# T921 · DM2/R1 角色包修复与 OP_48 栈诊断

2026-11-24 · Windows x64 Vulkan Release。

## 发布与原字封存

两族 split-v2 工件在 `.codex/local-test/t921-release/{dm2,r1}-split-v2`。每个目录内有同版 PE、`sand_assets`、owner 实际角色表收据和 `package.json`。CMake 将角色头和清单登记为生成目标，相关对象显式依赖生成头。清洁和修改生成头时间后的增量构建均成功，DM2/R1 均重编了 pass、owner 和 host。

DM2 与 R1 的 `package.json` SHA-256 分别为 `b2fc7207fdb97fe0a66eb573e46d1ed5746109fb46aadccbb7a0365e01fac1d7` 和 `09e6e4cc755ec2e62fab70ed4fc3c5e50efcc6a65ded1d105444f05e22cc8f6a`。

构建时选择 `-DSAND_FAMILY=dm2|r1` 与 `-DSAND_ROLE_VARIANT=t913-split-v2`。用 `python pc/vulkan/pack_sand_roles.py EXE BUILD/sand_assets NEW_PACKAGE_DIR` 发布。打包先暂存，调用 PE 的 `--check-package` 并比对清单、实物和 owner 收据，通过后才发布新目录。Windows 运行时同一份已核模块字直接送 Vulkan。文件拒绝报 opcode、路径、期望和实际字节及 SHA，先于创建该角色 Vulkan 对象。调用确有发生时另记原 VkResult、layout 指纹和时标。两族新包的 0x380 截短和等长修改负控分别得到 FILE_LENGTH、FILE_SHA。

```powershell
$d = '.codex/local-test/t921-release/dm2-split-v2'
& "$d/pc_sand_dm2_host.exe" --check-package "$d/sand_assets"
& "$d/pc_sand_dm2_host.exe" --modules "$d/sand_assets" --source INPUT128.u32le --source-timestamps-detail --raw-envelope OUT_RAW
```

`--raw-envelope` 在原业务门结束后从该 fence 及同代缓冲做冷拷贝。保实际上传的 INPUT128、GPU 镜像、原夹具、permit、Expected224、Completion256、配置、配额、初态、BASE/ACTUAL/REPLAY 和带代次及范围的 EVENT/PATH。R1 另保 device seed、visual maps、markers 和全 WORK。超期根若确已同 fence 退役，可封存在 `diagnostic` 且 `acceptedRoot=null`。未提交或未退役只记封存不可用。实际时钟、250 ms 业务期限和 typed 接受门沿原值。原根与冷拷贝分别记录 generation。

新包 BOOT 和 CPU/GPU 同根封存由 effector 做 PC 采证。开发核对覆盖编译、PE/角色包和本地文件负控。

## Intel OP_48

对照 T912 三组 ABBA 的 12 份监督收据和 24 个 minidump，按 owner TID 解 AMD64 保存的 RIP。共 23 个 RIP 位于 `igc64.dll`，另有一个 20 秒样本位于 `ntdll.dll+0x162114`。所有 45 秒样本仍位于 `igc64.dll+0x411080/0x411085`，两处相邻 5 字节。约 50.55–51.19 秒的成对读取中，owner CPU 增加 42.22–49.38 秒，且接近进程 CPU。各臂 `igc64.dll` 为 31.0.101.2140，SHA-256 `4860329b6002eb01fcc1f46c97f1551fb5acf7d651643bcb888a085f17168dd5`。

证据定位到 Intel 编译器内持续用 CPU 的长调用。单次 ntdll 采样无法确定系统调用或锁，minidump 地址也无法给出私有 IGC pass 名或永久卡死结论。本轮缺少 PE、snapshot 和资格前缀的分化，暂停前缀二分。下一对照固定单角色 OP_48 与同工件、同驱动、同监督窗，仅切现有 `--disable-optimization`，记录原 VkResult、owner CPU、20/45 秒 RIP 和实际映像。缓存隔离仍记 UNKNOWN。该旗标只用于独立诊断。

```powershell
python pc/vulkan/t921_op48_stacks.py test/reports/t912/runs-t916 OUT_OP48.json
python pc/vulkan/qualify_role.py .codex/local-test/t916-ninja/hello_world_pc.exe OUT_OP48_DEFAULT OP_48 --stack-dumps
python pc/vulkan/qualify_role.py .codex/local-test/t916-ninja/hello_world_pc.exe OUT_OP48_NOOPT OP_48 --stack-dumps --disable-optimization
```
