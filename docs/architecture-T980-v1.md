# T980 · 低分辨率流场预览与 shader 上屏

2026-11-25 · architect · 仅当前 Intel UHD 630。优先交 **16×16** 真网格的近似画面，接着复用同入口做 **32×32**。最短路径是复用已存在的私有 F02 GPU 投影和 OP9/10 的 GPU PACK，给 `PresentBoot` 加一个独立流场片元。首次交付采用单次求解后的稳定画面。正式 F02/F03/F05、业务 250 ms 资格和向 G14 提供可信 GridView 按原合同判定。 [T978 开发原卡](https://dida365.com/webapp/#p/6ac8a7e3e9adb60000000419/tasks/6b064946e4b0dfa9bee7d4f2) · [T828 正式合同](architecture-T828-v1.md)

## 实际求解入口和真实网格

| Profile | Cell / MAC faces | 压力层级 | 单 State / WORK / PACK |
| --- | ---: | --- | ---: |
| 16×16 | 256 / 544 | 16→8→4，52 sweeps/active V | 136,320 / 53,568 / 16,640 B |
| 32×32 | 1,024 / 2,112 | 32→16→8→4，60 sweeps/active V | 148,736 / 162,880 / 65,792 B |
| 64×64 原样对照 | 4,096 / 8,320 | 64→32→16→8→4，68 sweeps/active V | 198,144 / 597,632 / 262,400 B |

1. **计算烟测可以先于新窗口工件。** 原 PE 的通用 `--g13-fixture F02 CONFIG STATE` 按实际 Config 调 `FindProfile`、`ValidateConfig(true)` 和相应 State 大小，使用 `BootLayout` 为 compute shader 配置真实 `nx/ny/nf/level`。`F02-nonzero` 和 `F02-zero` 的专项原入口有额外 64×64 校验。通用 F02 在 `RunT833Candidate` 以私有路由自动设 `permit[93]=1`，输入文件内部是明确的 16/32 近似夹具。原 CLI 的 `--g13-trial` 仅允许 F03。使用 `--g13-split-op48` 的已采压力/黏性角色及 819 标量 seal。私有 J1 仍保两个有真实存储绑定的 ACTUAL/REPLAY、820 逻辑槽、同代比较和原封印。主要网格工作下降，128 KiB tracer 保留区、SHA、预排槽与冷启动依然有成本，16×16 的实际时延以集显计量。 [原配置与专用门](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/boot.cpp:1162) · [实际 permit](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:353) · [真实 Profile 表](../entry/src/main/cpp/gpu_resident/g13flow/contract_generated.h:4804)
2. **输入已交，直接复用。** `test/artifacts/t980/` 里有 16、32 各三组 `*-config128.bin`／`*-state.bin`。`circulation` 将 T964 的压力制造梯度加在闭盒顶点流函数 `ψ=sin²(πx)sin²(πy)/128` 的离散旋度上。投影应削去梯度、保留可视循环。`projection` 只含该梯度，作为压力非零、末速趋零的对照。`zero` 给静止负控。原边界墙法速和初始压力均零。16 正臂 480 个非零面，输入最大散度 0.04066287，独立离散参考在下/右轴向探点速度约 **+0.023918**、上/左探点约 **−0.023918**，压力约 **±0.248**；32 为 1,984 个非零面，参考对应速度约 **±0.024385**，压力约 **±0.2495**。这些是输入或独立参考，画面必须使用 GPU 的实际结果。原生 `ValidateConfig` 和 `BootLayout` 主机复核六组均为 diagnostic=0，当前近似夹具的 production 校验分别返回 5 或 4。 [夹具身份与指标](../test/artifacts/t980/fixtures.json) · [原生 host 校验](../test/artifacts/t980/verify.json)

现成计算烟测命令骨架如下。`$g13` 指固定的 T963 Windows x64 PE，`$run` 指新建的当次输出目录，`$fixtures` 指本卡夹具目录。首臂使用 16 文件，后续换对应 32 文件。该命令不带新增视图开关，预期只产求解证据。

```powershell
& $g13 --g13-candidate "$run/g13-preview-16" --g13-fixture F02 `
  "$fixtures/n16-circulation-config128.bin" "$fixtures/n16-circulation-state.bin" `
  --g13-diagnostics hot --g13-slot-snapshot --g13-tail-timing --g13-split-op48 `
  --g13-seal-stages --g13-seal-sha-scalar
```

## 上屏最小实现

1. **复用 PACK 和 Vulkan WSI。** 开发 T978 加显式 `--g13-preview`（或清晰的等价 flag），仅接受通用 F02 和配置实际 16/32。`--g13-candidate` 使 `bootOnly=true`，因此需要给该显式分支传入呈现资格，在 `OpenDevice` 启用 `VK_KHR_swapchain`，实际选到 graphics+compute+present 的同一队列族。原 `RunBoot` 在 `RunT833Candidate` 返回，后面的 `BOOT_DIAGNOSTIC_WSI` 是 Ctrl 的红绿状态，单开原 WSI 分支得不到流场。改在私有根安全退役后让该分支调用 `PresentBoot`。私有根的 `pack=4` 对应 `PACK1` GPU buffer。片元绑定 0 直接取这块 GPU buffer 和 `profile.pw*4`，沿 `render_pack.glsl` 的 64-word header、16-word/cell，`field[0/1]` 为格心速度分量、`field[2]` 为 GPU 动态压力。沿现 `boot_view.vert` UV 和单片元 SSBO 路径新增明确渲染分支，配实际范围与颜色读回，跳过绿像素专用判据。 [原 WSI 装置](../pc/vulkan/wsi.cpp:169) · [原 PACK 字段](../entry/src/main/cpp/gpu_resident/g13flow/render_pack.glsl) · [原窗口启用条件](../pc/vulkan/main.cpp:1033)
2. **完成态和帧隔离。** 先核原 nonce、fence 对象和代、真实成功退役、未撤销、同代 `T833TailMatches` 与 slot latch、Completion 无首错及双 scope、PACK header magic/seal/有效位、实际 `Nx/Ny`、nonce、config SHA 与 buffer generation。对晚 fence 单作 `PREVIEW_LATE`，在原始 `privateProjectComplete`、`acceptedRoot` 和业务结果之外登记 `previewPresented`；业务接受字段仍按原窗和 PRIVATE_PROJECT 语义。原 late 分支会冷拷后返回 `matched=false`，诊断启用时应在已验证冷档且资源仍 pinned 后独立给此 GPU PACK 视图，不能改写原 matched/nativeError，不能从已失败或未完成根取帧。`PresentBoot` 沿真实 compute-write→fragment-read 屏障、graphics fence、present 及退役持有资源。窗口/geometry 变化立即撤销本帧资格，首次失败显示明确状态。后续如保留上一帧，应带旧代和 stale 标记。 [真实 fence 与接受门](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:778) · [原冷拷与新 fence](../entry/src/main/cpp/gpu_resident/g13vulkan_boot/t833_candidate.inc:825) · [原图形可见性屏障](../pc/vulkan/wsi.cpp:348)
3. **首版画面。** 窗口标题带 `近似流场 · 16×16`／`32×32` 与 `及时`、`超期诊断` 或 `等待求解`。固定动态压力色标约 ±0.25，速度箭头或矢量纹理使用约 0.025 的尺度。场采样取真实 PACK cell，绘制可插值，另可显示原网格线辨识尺寸。模拟 y 向上时屏幕采样翻转一次，箭头 y 单独转为屏幕坐标；固定同一比例看边界和循环方向。首版允许持有一张真实 GPU 场。若需动感，再加仅在所采实际速度场内的 shader 示踪或多次独立真实求解，分别标注可视化时间与模拟步长；静态场随动画展示仍只有一个求解时刻。低分辨率模式启动器默认 16，可选 32，一次校验 PE、外/嵌 shader、Config/Seed，记清日志路径与各工件 SHA。32 不挡 16 交付。

## 验证与失败接续

- **T979 tester** 复用实际给定的三臂和原 PACK header。验证真实 16/32 网格、pressure J1 active cycle、原 u/v/pressure 有限及四墙法速为零、实际 cell 探点方向和压力对角符号。建议预览烟测容差为循环四点各同号且绝对值至少 0.010，输出散度 RMS 小于输入的 1/4，压力正负两象限各至少一对绝对值 ≥0.05。零臂检两场均接近零，纯梯度臂检速度显著降且保压力符号。独立查 wrong N、旧 64 PACK 缩画、输入 buffer 冒输出、零场代正臂、错 nonce/代/挡边、迟到冒业务和无实际 WSI 的负控。正式 T866 的零速度细阈值仍用于纯投影，不套到循环预览。效果证据用同根、同 buffer lease 的像素/图形 fence/图像与 actual N，截图单列。
- **Effector** 拿到 16 工件即在当前 Intel UHD 630 启动 circulation，记录同包强绑定、GPU/driver、实际 `nx/ny/nf/MG`、真实提交和 fence、pressure cycle、实际 PACK 字与第一帧时间、画面和边界。配对纯梯度和零臂。保 `250 ms` 相对记录，超期可显示诊断，真实未完成或物理拒绝保持具名状态。随后同方法换 32。若原私有 F02 在低域确实被固定阈值拒绝，先核原 `Config/Profile/Layout` 或具体 pressure seal 首差。需要限步或近似阈值时只在独立预览配置版本启用，日志记录预算、实际迭代和残差，原路径保留；仍非及时且已安全退役时先用已有晚预览。若原等待预算内无可用场，再以实测时间为输入决定独立轻量预览计划，避免以 64 场缩放替代。

已用原生代码做主机校验，并复核诊断调用链、实际 PACK 布局和窗口参数。尚未运行 GPU 或实际窗口。GPU 驻留，Windows Vulkan shader 上屏，禁止 Canvas 回退。 [夹具生成](../test/artifacts/t980/fixtures.py) · [host 校验源码](../test/artifacts/t980/verify.cpp)
