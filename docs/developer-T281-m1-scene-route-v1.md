# T281 GPU M1 scene 与独立产品路由开发交付

2026-10-30；developer；不使用真机。原卡 6ae34b06e4b069bbd86615b9。前置消费 T280 FullM1Transport，不等待 T298 独立验证。

## 实现

- `m1/scene.*` 实现 `GRS_GPU_SCENE_Q20_M1_V2`，直接读取 accepted publication：sand16、water20、column16。背景、床、活动沙、W 水体、真实 eta 水面、高光/透射、真实 J 圆片、前景在同一 lease/token、同一双缓冲完整帧中绘制。ARRIVE 后读取同一接受 publication，J 退包与 W 增量不跨 token 拼接。
- 水体只由 W 决定，J 不抬高 eta；深度透射、解析像素覆盖、水面法线/Fresnel 和光学参数沿 T221/T222/T226 合同实现。光学参数为 cold 配置并纳入 token；同 scene 改动拒绝。fragment SSBO 不支持明确 FAULT，无 CPU 池下载、纹理上传替代或 Canvas。
- token 包含全部 lease 身份、source/physics/frame/nonce、publication generation、物理/布局/角度/scene SHA、几何和光学参数；过期 publication、错 epoch/context、失败 swap 均不提交 LastGood。每次消费归还带 GPU fence 的 lease。普通路径仍仅映射 1024B completion。
- 新 `m1::Run` 使用既有 Fixed120Clock，串行调度 SOURCE 和完整 QUANTUM；不改 delta、时钟间隙判定或物理数学。新 `libgrsm1.so` 单独编译同一生产 app_bridge，初始化使用 FullM1 manifest/组合 SHA，水源保留 revision2 原始 32 words。M0 继续原 revision1 验证。
- `GpuResidentM1.ets` 是独立 XComponent+shader 页面，由 Ability Want `grsm1=true` 进入。提供砂/水源、角度、新空场两档入口及 READY 身份门。`configureM1(K,Kw,features,revision)` 精确接受 K192/512、Kw64/128、3/2；新空 run，不复用 M0 reader/水 ID 哨兵。M0 默认入口、旧 F64 和后续 T228 交互职责保持原边界。

## 非机证据

`test/reports/t281/delivery.json` 为机器清单，保存组合/scene SHA、签包 SHA、模块及四档结果；`pixels-*.json` 保存每档 35 张 RGBA 层图的 SHA。原始 RGBA 在 `.codex/local-test/t281`，可由脚本重跑生成。`host-scenes.png` 供查看五场景完整帧。

| 检查 | 结果 | 范围 |
|---|---|---|
| 真实 GLES3.1 scene 四档 | 196/0（各49） | ANGLE SwiftShader；真实 FullM1 GPU SOURCE/QUANTUM/accepted publication/lease/scene；空水、深水、飞行、ARRIVE、活动沙、错 context/旧 publication/坏光学/失败 swap |
| 独立像素 oracle | 20/0 | 深水线性 premult→sRGB 独立计算≤2LSB、干邻列、W0、退包不双显、J 可见 |
| M1 controller/packet | 7/0 | 真实 Run/Fixed120Clock，transport 明确桩；rev1/rev2隔离、水包原字不变、完整 quantum 调度、时钟拒绝 |
| 生产 app_bridge + 真实 GL | 5/0 | 同生产 loop、真实 FullM1 与 scene；PC pbuffer 替代原生窗口、脚本时钟；sourceCommit1/physicsCommit2、两次 accepted、CURRENT |
| M0 bridge 回归 | 16/0 | 真实原 Run/Clock，窗口/EGL/transport 明确桩，未冒充真实 GL |
| hvigor PackageHap | BUILD SUCCESSFUL | grsm0/grsm1 均编译入包 |
| 签名/验签 | PASS/PASS | 未安装设备 |

scene 专项 fixture 将 kappa=0 以隔离 ARRIVE/退包像素，未改生产 kappa24 默认；完整默认物理证据沿 T280，不以此截图替代其验证。离线 app 测试采用脚本时钟避免 PC shader JIT 被误作设备调度结果，不证明真实设备首帧 delta。

执行入口：`prepare_t281_scene.py` → `build_t281_host.py` 生成 MSVC build.cmd → scene.exe 四档 → `check_t281_pixels.py`；`build_t281_run.cmd`；`build_t281_app.py` 生成生产 loop GL 构建脚本；`archive_t281.py` 核对证据及 host/产品 scene shader 完全相同。复用工程已有 ANGLE/SDK/MSVC 离线环境。

签包：`.codex/local-test/t257-sign/T281/T281-signed.hap`。

SHA256：`2025d193f19bda4543354ec8c5f99cf6120d12d07d2884a9a7bac4ddf0a7c6a8`。

交付为开发、编译及非机自测完成；真机安装/端到端/光学效果/性能收益均 NOT_RUN 或 NOT_EVALUATED，另交 tester/effector。不将 PC 软件 GL 记为设备证据或产品 PASS。
