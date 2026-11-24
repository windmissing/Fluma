# T918 · 紧凑 Demo

2026-11-24 · Windows x64 Release。通过 `tools/start-pc-compact-demo.ps1` 启动；`-Baseline` 用同一程序和原三份物理 shader，`-CheckOnly` 仅核工件。

```powershell
.\tools\start-pc-compact-demo.ps1
.\tools\start-pc-compact-demo.ps1 -Baseline
```

在盒内持按供沙。紧凑档左右键每次转 5°，请求范围 ±30°，到边界后可反向。R、Home 和右上回正按钮均可用。画面和指针换算共用盒体映射。标题区显示盒体预览角、实际角和待应用/限角状态。空盒在首笔物料前即可重绘；有堆积时可用上一个同根 GPU 发布态先重绘盒体。这类重绘记为 `M1_BOX_VIEW_PRESENT_QUEUED`，带原根和实际角。业务帧另有 `m1GravityFrame`，只有物理接受才推进物理账。跨 CLOCK_GAP 沿 T916 授权恢复。

紧凑物理有 80 个可用列 `[24,104)`、64 单位高。128 列 ABI 保留，外侧为零库存填充。颗粒运动与独立 replay 同时实施内侧墙及顶界，水和床通量不能越墙，坡度判据保留在各实际内面。落点按鼠标的盒内坐标，高处指针将最大出生高度压到 16。SOURCE 请求档从 8 提到 32 单位/秒，沿实际相邻脉冲时长计量，仍由容量、槽位、配额与 GPU 守恒核查决定实收。床、颗粒和水始终从 GPU 发布绘制。

## 工件与验证

程序 `.codex/local-test/t918-ninja/hello_world_pc.exe`，SHA-256 `f5aff05df12f837edd81852b742e7a6d41f50d1b408e96c6eda74d569ff8262d`。紧凑物理三件在 `.codex/local-test/t918-shaders`，完整 SHA-256、shader 字节及绑定见同目录 `m1_static.json`。从源码生成三份物理 shader：

```powershell
python test/gpu_resident/prepare_t280_commit.py
python pc/vulkan/build_t918_compact.py --assembled .codex/local-test/t280 --compiler .codex/local-test/g13-glslang/bin/glslang.exe --out .codex/local-test/t918-shaders
```

Windows Release、三份物理 SPIR-V、原三模块哈希对照、主机指针映射断言及两种启动工件核对通过。映射断言覆盖正负角、边界拒绝和出生高度。两秒至四秒见堆、真实落床及松手后留存由 effector 在 PC 上采像素与 SOURCE/LAND 同根原件，当前交付为可运行体验档。
