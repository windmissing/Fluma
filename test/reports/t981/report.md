# T981 · 目标13与14十六格近似预览集显验证（effector）

2026-11-25 · 开发集显 PC（Intel UHD 630 `8086:3e98`，driver 31.0.101.2140/1656924，Windows x64）。GPU 驻留、Vulkan WSI + shader 唯一上屏，无 Canvas 回退。仅 16×16。

工件全核：T978 包 21/21 件 SHA 与 manifest `ea229bef` 逐字一致；T984 包 12/12 件 SHA 与 package.json `9ceab412` 一致（PE `c850b5da`、g14 fragment `acb7248f`）。启动器与 `pc_g14_preview16.exe` 直接可复跑。

## ① 目标13 · G13 16×16 私有流场预览（T978/T980 合同）

三臂各跑 `Start Preview.cmd`（16×16，once 收尾 + 持续窗口各一次），全部走到实际求解、真实 fence 退役、冷档落盘与窗口 Present：

| 臂 | 非零输入面 | GPU peakSpeed | oracle 峰值 | 压力范围 | oracle 压力 | PACK1 SHA |
|---|---|---|---|---|---|---|
| circulation | 480/544 | 0.0236333 | 0.0239182 | ±0.247996 | ±0.2479963 | `bcd10fe9` |
| projection | 480/544 | 3.519e-08（衰减 6 个量级） | — | ±0.247996（符号保留） | — | `90011f84` |
| zero | 0 | 0 | 0 | 0 | 0 | `3315d38f` |

- **网格与求解真实**：`g13CoarsePreview` 记录 grid=16、cells=256、macFaces=544、mgLevels=3；PACK1 头字（magic `1446195527`、words 4160、Nx/Ny=16、nonce、configSHA）过 `G13CoarsePackMatches` 全检，256 格 field 全 finite，非零臂 256/256 格速度非零。
- **方向判据（T979）**：circulation 四探点 bottom+ / top− / left− / right+，与 oracle 探针方向逐点一致；压力呈乘积型符号分布（Q(++) max +0.248、Q(+-) min −0.248，≥0.05 判据过）；projection 速度被削至 3.5e-08 而压力符号保留；zero 两场严格为零。
- **fence/晚尾**：三臂根 submit→fence 实测 908/1017/885 ms，均超 250 ms 授予期 → `GRANT_DEADLINE`、nativeError=9、`acceptedRoot=null`，按设计走 `PREVIEW_LATE` 呈现路径（`previewPresented=true`、窗口标题“超期诊断”），未冒充业务接受。GPU spans：root 907.8 ms（actual 521.5 + replay 358.3）。
- **窗口与像素**：WSI Present vk=0、graphics fence=0、imageIndex=0、格式 44；`.preview-pixels.bin` 768B 三区 SHA 与 Present 行逐字一致（circulation `a6e8f34f`）。实际窗口截图（`window-shot.png`）：左半速度矢量图青色箭头 ~11k px，右半压力区暖/冷像素 12.7k/17.5k，橙色超期标志可见。目标13 正式资格不受影响，`acceptedRoot` 保持空。

原件：`runs/g13-16-circulation|projection|zero|circulation-live/`（各含 preview.jsonl、.quantum 574,064B 冷档、.preview-pixels.bin、manifest）。

## ② 目标14 · G14 16×16 解析移动床三臂（T982/T984 合同）

三臂各跑 headless（30 根冷档）+ `--present` 可见模式（PrintWindow 连拍：moving 13 帧、fixed/mirror 各 9 帧）：

**T982 oracle 逐项实测对照（全部通过）：**

| 校验项 | oracle | fixed 实测 | moving 实测 | mirror 实测 |
|---|---|---|---|---|
| 湿面积 m² | 17/3200 = 0.0053125 | 0.005312500000000005 ✓ | 同 ✓ | 同 ✓ |
| 中线累计通量 m² | 0 / −1/6400 / +1/6400 | 0 ✓ | −0.00015625000000000006 ✓ | +0.00015625 ✓ |
| 沙侧压力冲量 x (N·s/m) | −0.00185546875 | ≈0（6.4e-21）✓ | −0.001855468750001 ✓ | +0.001855468750（反号）✓ |
| 沙侧压力冲量 y (N·s/m) | +0.003125 | +0.003125 ✓ | +0.003125 ✓ | +0.003125（同号）✓ |
| 切向黏性冲量 x | 独立原值 | 0 ✓ | −1.046745044842e-04 ✓ | +1.046745044842e-04（反号）✓ |
| 传输残差 max (m²) | 2.753e-21 | 0 ✓ | 4.235e-22 ✓ | 5.294e-22 ✓ |
| 几何残差 max (m²) | 同量级 | 0 ✓ | 1.258e-19 ✓ | 9.106e-20 ✓ |

- 冲量从 30 根冷档 `root-*.u32le` 的 audit 段按 FP32 pair 逐根累计复算，x/y 与 oracle 逐位一致（第三位小数级）。
- **画面演变（连拍逐帧 diff 非空）**：moving 床 M+ 旋转左降 +12px/右升 −9px；mirror 反向（左升 −5.9/右降 +6.0）；fixed 床零位移。三屏布局（湿格+流速 | 压力 | 剪切）蓝色水域 `(24,92,135)`、棕色床 `(79,50,27)`、亮青流速箭头 `(181,245,255)` 均可见，末帧静止=运行自然收尾。
- **Present 链**：三臂各 30 根全部 acquire/submit/present vk=0，fragment SHA `acb7248f`（包内 manifest 一致），`acceptedRoot`/`typedToken` 按独立近似预览身份保持空。
- 设备门：`G14_DEVICE` 记录 shaderFloat64=true（UHD 630 实际支持），vendor 32902/device 16024。

原件：`runs/g14/{fixed,moving,mirror}-{headless,burst}/`（各含 roots.jsonl 30 行、input-*/root-*.u32le 冷档、present.jsonl、连拍帧）。headless 与可见模式末根数值逐位一致（重跑稳定性）。

## 结论

- 目标13：16×16 非零实际 GPU 流场 + 可见首帧已采，circulation/projection/zero 三臂方向、幅值、压力符号、零负控全过 T979 烟测判据；根均晚（250 ms 期限），按 PREVIEW_LATE 如实呈现，正式资格字段未污染。
- 目标14：16×16 三臂守恒、镜像反号、冲量、残差逐项过 T982 oracle，可见画面证实移动/镜像/固定三种床运动形态。独立近似预览身份维持。
- 用户可直接启动：G13 用 `.codex/local-test/t978-g13-package/Start Preview.cmd`；G14 用 `pc_g14_preview16.exe g14-preview16 modules.json <arm>.g14 <新输出目录> --present`。

## 环境

| 项 | 值 |
|---|---|
| GPU | Intel(R) UHD Graphics 630（8086:3e98） |
| 驱动 | 31.0.101.2140 / 1656924 |
| OS | Windows x64 |

报告：`test/reports/t981/report.md` + `summary.json` + `runs/` 原件。
