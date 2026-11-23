# T886 · PC 三态与真实通量的独立接入切片

2026-11-23 · developer · Windows x64 Release。两族按自己的冻结输入独立生成、编译和链接。编译、SPIR-V 校验、反射及离线派发/复制范围检查已通过。

| 切片 | Release 库 | 库 SHA256 | 角色集合 SHA256 |
| --- | --- | --- | --- |
| DM2 / K192 / S40 | `.codex/local-test/t886-dm2-release/pc_sand_dm2.lib` | `24b26e02ff95f628ed16f12224958b515ae2663b790c1a6156cf5a1dc7b27b62` | `3639e57f834308bab001ca783f321b6a503c232caa924e04b1d8f1d7e3333ca6` |
| G11 / R1 / K192 / V2048 | `.codex/local-test/t886-r1-release/pc_sand_r1.lib` | `48f28e28f8382243847fca6aebcb3ddecf46ba11ed71b8020cc3759257881b2f` | `f7d812d64eba3c932fcfc41d70fbed48835349819f235b6d9d61fb58216641f3` |

## DM2 · 目标 9

- 使用原 DM2 生成器。45 个独立计算模块覆盖 BOOT、12 槽 SOURCE 和 2404 槽 QUANTUM，原掩码为 `0xffff / 0x3ffff`。Vulkan set 0 的四个 storage binding、240 B Common 与 20 B push 已反射核对。最大 GLSL 为 43,302 B，最大 SPIR-V 为 168,876 B。
- PC 角色接缝从正式表取逐 ordinal 参数，沿原 CURRENT/BASE/CANDIDATE/SELECTED/REPLAY 槽出绑定并记录计算和复制。ROOT/FULL 原档适配器消费本族 typed accepted proof、真实缓冲范围和 Vulkan copy fence，同步、非一致内存失效与映射后流式写出原 `G9_SPF_DM_COLD_V2`。K192 ROOT/FULL 分别为 26,750,976 / 103,354,368 B。FULL 需要调用方在根开工前预留六个 discovery arena。
- 接续需要本族 Vulkan owner 创建并确认实际 pipeline、BOOT 数值与 S40 资源资格，再用真实 SOURCE/QUANTUM 接 typed tail 与同代 state/arena pin。PC 没有本族合格接受根，因此本轮原档工件是写入接口和范围计划。独立 reader 接续时按 Vulkan 角色集合及实际封尾模块 SHA 对账。现有 GLES reader 的默认 shader 身份分支按历史用途保留。

## R1 · 目标 11

- 直接使用 R1 组合父 writer，通过独立 R1 生成入口完成 49 个计算模块及三组底材、glyph、packet 共六个图形模块。保 19/2543 槽、`0xcffff`、304 B Common、36 B 计算 push，以及原 ACTUAL/REPLAY。最大计算 GLSL 37,318 B，最大计算 SPIR-V 170,728 B，最大图形 GLSL 1,830 B。R1 生成路径独立于组合 DM 分支，原双族生成入口仍沿原合同。工作区的组合 DM 分支存在旧 `G11_WRITER_ANCHOR` 漂移，R1 单族入口无需经过该分支。
- 按真实 WORK 字节范围生成单批 `G11_PHASE_WITNESS_V2` 复制计划。离线对照定位到 R1 micro 0 FLOW 的 ACTUAL 699→702、REPLAY 1965→1968。窗口接口只取 R1 联合 typed proof 及同代 state、marker、glyph、Pstyle，沿实际图形 fence 和 WSI 回执生成 `PresentedMapping`。glyph 绑定保 loc 0/1、U32x4 与 stride 32，图形模块使用负高 Vulkan viewport 和原业务坐标。
- 接续以实际 R1 device/role、原父根与联合发布通过为前置。由真正的 owner 固定受理根和资源代、执行单批复制与提交，再由 PC 取证角色用真实有符号非零流、零流/反向、同 lease 画面及 WSI 原件验证。当前交付为可编译模块及调用接缝。

## 独立重建

在 MSVC x64 环境中运行以下命令，第二族改用 `-B out-r1 -DSAND_FAMILY=r1`。每个构建目录的 `sand_assets/roles.json` 列出父源、全部 dispatch、模块、SPIR-V 字节与 SHA、反射布局及 `NOT_EXECUTED` 硬件状态。

```powershell
cmake -S pc/vulkan/sand -B out-dm2 -G Ninja -DCMAKE_BUILD_TYPE=Release -DSAND_FAMILY=dm2 -DG13_GLSLANG=<glslang.exe> -DVULKAN_HEADERS=<Vulkan-include>
cmake --build out-dm2 --config Release
```
