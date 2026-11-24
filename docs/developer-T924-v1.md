# T924 · DM2/R1 同宿主分版与 SOURCE 录制准备

2026-11-24 · Windows x64 Release。六个新包位于 `.codex/local-test/t924-release`，各族有 `baseline`、`t913-v1` 和 `t913-split-v2`。各包均带 `package.json`、owner 角色表收据和原模块，启动行带 `hostReceipt=T924_V1`。T921 旧包仍可使用其原路径。

| 族 | baseline package.json SHA-256 | t913-v1 | t913-split-v2 |
| --- | --- | --- | --- |
| DM2 | `911ae4b9873e7e89b8a87df2e19674151c34dfc2997cc1d8c71d1c51f29aa829` | `3d20ab485164710cd3a47998d8541e78b6f77d668d8f1c5dd4f030dc033b02a4` | `c21c5b2dd0e1e1c97d39906c8b9915c19331d1ce5583701dcb2cf7c249170228` |
| R1 | `13d47cb4163fad2a7ade73023e7c09a3a1e1eb6e63099dda8db94be0209f40c2` | `2c933767b76ad2709cb5e5559a49d1cf26da3048348c779100cacb28effe9343` | `0da0d0cbea137d2966dc1d85a30dd3533d8d2d63a60aee3e43c724241e8418ec` |

## 同版封存与配对

六包沿各自冻结 roleSet 与 SPIR-V。实际去重、Actual/Replay、ROOT、raw envelope 和冷退役格式兼容 T921。以同一业务输入、参数、设备和诊断开关配对。保完整 INPUT128 与 GPU_INPUT128，对照使用原始 SHA 和真实时钟。在查看结果前先用 T922 核 PE 对应角色、时标与各臂独立资格。

```powershell
$base = '.codex/local-test/t924-release'
$a = "$base/r1-baseline"
$b = "$base/r1-t913-v1"
python test/t922/check.py package "$a/pc_sand_r1_host.exe" "$a/sand_assets" baseline
& "$a/pc_sand_r1_host.exe" --modules "$a/sand_assets" --source INPUT128.u32le --source-timestamps-detail --source-prewarm --raw-envelope RAW_A
& "$b/pc_sand_r1_host.exe" --modules "$b/sand_assets" --source INPUT128.u32le --source-timestamps-detail --source-prewarm --raw-envelope RAW_B
python test/t922/check.py diff ACCEPTED_ENVELOPE_A ACCEPTED_ENVELOPE_B "$a/sand_assets/roles.json" "$b/sand_assets/roles.json"
```

## 录制准备对照

两族的 SOURCE `ROOT_TIMELINE.commandPreparation` 分别记录 `vkResetCommandPool`、`vkResetFences` 与 `vkBeginCommandBuffer` 的真实 ENTER/EXIT 和原 VkResult，保 command pool、fence、上一个代及使用代。默认三调用仍在 grant 后。固定同一 R1 包、设备、输入、时间戳档和 descriptor prewarm 设置，仅在另一臂增加 `--source-prepare-retired`。

提前准备只在 Vulkan BOOT 资格成功、owner idle、原 fence 实际确认成功后进行。在申请 SOURCE grant 前完成三调用。原 fence 对象复用，正式录制只消费一次该准备，并在实际录制时建立对应使用代。Expected、INPUT、upload、dispatch、submit 和 live guard 沿真实 grant，仍用 250 ms。提前准备失败给原 VkResult 并中止该臂。未提交或已拒绝的根按原归档口径。前置事件和根尾收据可用 `python pc/vulkan/check_t924_command_preparation.py RUN.jsonl` 核对；T922 单独核真实接受。

六包 Release 与独立 PE 包检查通过。计量自测覆盖 grant 内/提前两臂及错误代、提前前未退役、Vulkan 调用失败三项负控。实际主机时延、续根和跨包已接受原字由 effector 复采。
