# T364 双态单沙材质开发交付

2026-11-06。按 T362 v1 实现显式 `singleSand=1` 入口。进入 V2 隔离页后，封签 publication 交给独立 SingleSand controller，沿页面已有 XComponent 会话和 EGL owner 绘制。原 0/1/2、base-strip、默认页面和物理路径维持原协议。

- 独立配置、资产和 frame packet 走三个新 NAPI 入口。配置校验包含有界线性增益、原始 F64 位型 digest 和版本闭集。NAPI 先核 typed view 及互相重叠，再取得自有 packet 并对 display、pb 和投影字节重算 SHA256。
- 封签准备检查 V2 dry envelope、单 H 库存及活粒 phase，复用列保持投影。S_D 执行一次 DISPLAY_4X3，pb 从其 F64 显示序列计算高斯及端点差分。原始字段支持字节和显示 F32 哈希分别记录。粒子 state 1 在中心进入同帧显示床时整粒隐藏，state 2 保留几何。
- R8 冷生成共用原始 raw 算子，保留原循环顺序。特征 2 的 R8 SHA256 为 `3b0a40d6502e6fa234915cd2581c2795e2ed65cee5724a738d4279c37c5e0a3b`。特征 2/3 的原始 F64 tile 分别与基线逐字节对照通过。
- 原生按 config/asset generation 取得 lease，在 sRGB candidate 上按背景、床、粒子、前景绘制，恒等采样送入 EGL 目标。swap 成功后切换 candidate 和 active 资产。注入 swap 失败可回放完整旧帧并标 STALE，context 改变后用已保存 R8 字节重传。运行路径没有像素读回。
- 收据在显式按钮冷导出。包括请求与实际 token、素材与 profile 哈希、原 GL/EGL、目标颜色信息、粒子及床支持诊断、lease、上传字节、绘制次数和原生总 wall 时间。CPU 遮挡计数是中心判据诊断，精确覆盖和裁剪像素留给 T365 的独立读回。

本实现使用一份 ArkTS R8、原生 active/pending 各一份 R8 和最多两张 GPU R8。已知上界为 327,680B。profile 按一份 ETS F32、一份原生 candidate F32、一张 GPU R32F 计 3,072B，已渲染的 LastGood 由独立目标保存。F64 profile 三个 128 列缓冲加权重为 3,368B，hash staging 为 18,432B。实例在 native 使用最多 8,192B GPU buffer，另有 ETS 和 native 输入副本。两个完整 SRGB8_ALPHA8 目标是 8WH 字节。上述账目分列了渲染状态，未包含驱动、对象、hash 临时内存或 256² F64 纹理生成峰值。常态只上传 1,024B profile 和 16B/活粒，冷重传另计 65,536B。

离线检查见 `test/reports/t364`。Node 核验封签空场、H、CONTACT_WAIT、湿/L/身份拒绝、显示 F32 和旧 raw 逐位对照。生产 GLSL 在 Windows GLES pbuffer 编译并实际绘制，sRGB FBO 的床和粒子中性色精确为 145/168/111，增益、接触遮挡、薄床、注入 swap 失败与 context 重传均通过。pbuffer 读回为线性码 72/100/40，背景为 1/1/4。它与 FBO 存储读回分开保留。真实 EGL 窗口及独立 SM01–12 由后续角色验证。

旧 Scene 5/5、base evaluator 13/13、原生 Scene 原子性和 base-strip 核心回归通过。V2 publication、material preparation 和 exact70 既有检查通过。工作区原有 CMake 缓存曾生成重复 linker flags 的异常长链接命令，随后从同源干净工程完成 Hvigor PackageHap 并验证签名。签包位置、SHA 与构建证据见本卡归档。设备未运行。
