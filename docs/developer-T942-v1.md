# T942 · 集成 Demo 落点、重力与 G15 角色包

## 集成 Demo

集成模式以真实提交并呈现的 CompactView 逆映射 Win32 客户区像素中心。逐帧记录图像范围、DPI、viewport、scale、中心、实际三角系数、请求和接受角、swapchain、graphics fence、publication 及变换版本。身份与尺寸匹配的 Win32 identity surface 直接使用物理像素。映射无证时暂停入料并提示「画面更新中」。

SOURCE 继续消费原 Binary64/Q20 接口，集成模式准入盒内实际高度，去掉旧 y=16 上限。实际映射变化撤销旧预约，仍捕获的持按会在 UI 线程重采一次光标坐标和 QPC，再从新相邻真实脉冲生成预约。盒外要求重新按下，床下或容量不足按 SOURCE 的实际 GPU 拒因反馈。新指针样本的首次有效出生优先呈现含该出生的首个可用 Q 帧。

盒框、床和粒子使用 pinned QUANTUM 的 Completion256 word 107。首张 READY 空盒保持水平，无 SOURCE 的转盒请求用真实 grant 提交 QUANTUM，接受后才改变画面。箭头按屏幕 Q20 (0,1048576) 绘制。请求、实际角、床形限制、角域上限、待应用和上一有效帧分别报告。R、Home 与同帧按钮共用回正输入。物理常量、床形保护、1/120 s、真实 fence 和 250 ms 门沿用。

可复建命令在 [T933 交接](developer-T933-v1.md)。将 Windows Release 输出改为 `.codex/local-test/t942-ninja`，用原 35 派发外置模块重新打包：

```powershell
python tools/package-pc-integrated-demo.py .codex/local-test/t942-ninja/hello_world_pc.exe .codex/local-test/t933-shaders .codex/local-test/t942-integrated-demo
```

送验包唯一入口为 `Start Demo.cmd`，`manifest.json` 固定 PE、外置计算模块和内嵌图形模块身份。MSVC x64 Release 与 SPIR-V 验证通过。C++ 离线对照核 5 种 viewport、7 角和 1,400 个 Q20 回投点，核最初 T935 中央点的 y=32、高位取样、向下投影及变换失效。实际出生/首粒同根像素、DPI 矩阵、倾斜后自由段、4 秒见堆、60 秒继续出沙和留存沿 T940/T935 做独立 PC 复测。体验包待复测后放行。

## G15 正式角色包

`g15-dm2-op23` 按 T937 证据只删除完整编译产物 opcode 23 的一条 52 B `OpSourceExtension`。构建要求原 SPIR-V 的 SHA 精确匹配 T937，且去记录结果与已验证 DM2 baseline 的 SPIR-V 完全一致。重新生成角色清单、编译时 SHA 门和配套 PE。45 个角色中只有 opcode 23 的 SPIR-V SHA 改变，派发计划和角表逐字一致。旧 PE 与旧角色包可作为历史对照，双向混配在 opcode 23 的 `ROLE_REJECT` 处离线拒绝。

Release 包 `.codex/local-test/t942-g15-package` 的 roleSet SHA 为 `1bb757fc2f1af27dd76340f3fafeb7cf15d9e3eb607ee5ce18082d88308db463`，`package.json` SHA 为 `e31361c20bfa24b8269f7e7d9940982f9fc81c6218cc25c7893b6c66d78dc82b`。实际正反砂丘、typed QUANTUM 与同根 shader 由 effector 使用该包采证：

```powershell
$g = '.codex/local-test/t942-g15-package'
& "$g/pc_sand_dm2_host.exe" --modules "$g/sand_assets" --g15-dune --g15-tilt-q8 11520 --steps 240 --raw-envelope T942_G15_POS --present
& "$g/pc_sand_dm2_host.exe" --modules "$g/sand_assets" --g15-dune-mirror --g15-tilt-q8 -11520 --steps 240 --raw-envelope T942_G15_NEG --present
```

重建时以 `-DSAND_FAMILY=dm2 -DSAND_ROLE_VARIANT=g15-dm2-op23` 生成 Release，并用 `pc/vulkan/pack_sand_roles.py` 验证入包。开发没有启动 PC 端到端场景。
