# T370 · 单沙像素缺证分流与取证合同 v1

2026-11-06，architect。依据 T365 正式报告与原卡回执、T364 `22d1f3a` 的 host/ETS 证据、T356 像素合同和 T362 工程合同。本轮离线核档和源码。后续 tester 可先开离线像素卡。窗口探针由 developer 单独交付，真机卡消费该签包。

## 1. 原证据裁定

T364 签包 SHA256 实核 `33d395932c75811b5b368a2e5c500cf9a7b6df173097ae8e97831fb43acf56b3`，交付登记 shader SHA256 `f22a5e867ec64e3be2fc649596a69a271f1ec143a89b923f001cb982b7ae0993`。归档 `test/reports/t364/host.txt` SHA `37a7df2f…a049`，`ets.json` SHA `2dd35d12…0edb`，T365 复跑件 SHA `354e3e96…faac`。对照两个 ETS 文本中的 `generatorHash` 和 `tiles[]`，特征2的 `3b0a40d6…c37c5e0a3b`、特征3的 `ba3f3d1b…` 及生成器身份相符。T365 独立执行了既有 `test/t364/units.cjs`。该证据支持相同断言和生成结果的可复跑性。SM 像素正反控仍需独立公式、ROI 和实际读回。

`test/t364/host.cpp` 调用生产 `SingleSandPass::Draw`、同一 GLSL 和 `FullFrameTargets`，在 host GLES 实际渲染。`TestCommittedFramebuffer()` 返回已提交 sRGB FBO。三个断言点验证床、AIR 的中性码 `145/168/111/255` 以及背景 `16/16/36/255`，保留 SM01 的中性单点正控。gain、CONTACT_WAIT、state1 遮挡有追加单点断言，薄床有 `bedSupport` 断言。它们支持有限正控和元数据路径。SM02 的光学逐像素域、SM04 的纹理分布、SM07/08 的精确像素覆盖、SM10 的三次整帧位型仍缺证。SM11 保留现有封签/身份、host 注入 swap 失败后的旧帧重放与 context 重传结果。

SM12 必须按实际读点归档。host 的 `EglContext::Swap` 是替身，读取 pbuffer 默认 framebuffer 的 PRE_SWAP 样本，日志给出 `72/100/40/255` 和背景 `1/1/4/255`。host 实际请求 `EGL_GL_COLORSPACE_SRGB_KHR`，还断言 `GL_BACK` 的 attachment encoding 为 `GL_SRGB`，且预置断言接受两套输出码。`72/100/40` 是该读点观察值，单凭其数值不能确认中间目标是线性 `GL_RGBA8` 或定位转换位置。该 host 没有独立的线性 FBO 校准，也没有真实窗口交换。SM12 登记为 **sRGB FBO 中性单点已证、pbuffer 默认读数在档、线性 FBO 独立校准和真实窗口待测**。保留 T364/T365 原文件和已测结论的窄范围，T370 作为证据口径补注。避免用“7/12”合计遮蔽各项的部分通过与未测点。

## 2. 先做离线 FBO 的最小矩阵

复用 `test/t364/prepare_host.py`、`build_host.cmd` 的 GLES 环境及 `P013_SINGLE_SAND_HOST_TEST` 已有 committed FBO 访问点。由 tester 在独立测试目录消费原 `single_sand_pass.cpp`、`single_sand.vert/.frag`，保留 shader SHA。新增完整 ROI `glReadPixels(GL_RGBA, GL_UNSIGNED_BYTE)` 冷读回。常规输出从真实 sRGB candidate 成功提交后读 `TestCommittedFramebuffer()`，校准控制另建线性和 sRGB FBO。固定同一 renderer、fixture 源字节、config、box 和 ROI，独立 oracle 只读最终 typed profile/xy/meta/R8 与显式几何，按 T356 公式求像素。source hash 和 DISPLAY 输入/输出身份另留，支持将 profile 准备错误与 shader 错误区分。test packet 中 synthetic `rawHash` 标明 `HOST_SYNTHETIC`，产品封签路径仍以既有 T364 ETS 回归为证。

| 项 | 最小正控和反控 | 读数与归类 |
|---|---|---|
| SM03 背景可见 | 无床，盒内背景常量，至少三枚稀疏不重叠 AIR 圆片，中心离边框至少2r。正常 gains、零 gains、独立同色背景反控。反控可用只在夹具中覆盖的统一 uniform，生产配置的固定 palette 校验继续执行。 | 固定像素中心掩膜和背景对照，按目标格式将通道转到线性后算 CR P05/P50，再报 Ycode 差和有效像素数。背景反控用中性增益，CR 应接近1。CR>=4 仍是候选线。 |
| SM05 床面明暗 | R8 128 或 grainGain=0，flat、正坡、负坡、镜像，并有同一 S_D 的 bh/mh 置换。surfaceGain OFF/ON，固定预选 x，测 0/4/8/16 logical 深度附近合法原生像素中心。正坡及负坡用可判别的非零 faceDelta。反控让 oracle 以错误单位或错误倍乘 s 预测。 | `d=(S_D-h)*H_native/capacity` 后按实际采样中心算 exp，先报实际深度，d=0 若无合法中心用最邻近床内点并单列。flat ON/OFF 逐像素相同。保留同 S_D 的 pb 字节、材质配方输入和列端点插值。 |
| SM06 同色重叠 | flat 床，放一枚完全位于床内的 state2 圆片，另加一枚远离它的 state1 中心遮挡对照。用同一 profile/xy/meta、同一 config，`packet.enabled=false/true` 生成诊断 BED_ONLY 和完整双态。另组 `grain/face/core/rim=0` 做同色无细节反控。 | 使用几何独立生成的每 ID 可见支持集，计算 `D=abs(Ymix-Ybed)`、样本数/遮挡率及 D>=4 比例，剔除露背景点与前景边框。反控 D=0。>=8 像素且40% D>=4只记候选。每个 arm 独立递增 frame serial，明确同输入配对，不复用帧身份。 |
| SM09 尺度和相位 | 64×64 logical box，native 分别 64/128/192，s=1/2/3，半径2s。每尺度四种原生中心分量相位 (0,0)/(.5,0)/(0,.5)/(.5,.5)，用已存 Float32 material xy 回算真实中心。至少一个离轴镜像臂和故意二次 scale/双镜像反控。 | 比对逐原生像素的掩膜、中心和径向亮度。半开盒边与 `rho<=1` 按像素中心算，独列边界临界点及精度不确定性。纹理和床面深度随同一 s 采样。像素个数有栅格相位差，按几何 oracle 评估。 |
| SM02/04/07/08/10 剩余像素 | 复用上述 ROI，增加最暗/最亮配置、生产 R8 字节的 flat 床纹理 ON/OFF、空/0.15薄/厚床、两个重叠粒和同 fixture 三次冷/热绘制。局部透明输出需要独立透明清屏只画指定层。 | 在已验证的线性目标检色度与通道范围，在 sRGB 存储码统计纹理 ΔY RMS/P01/P99，按层测 alpha/mask，成品背景自身不透明。静态三帧帧序列改变而完整像素 hash 恒等。RMS候选0.5–8码单列，所有未覆盖测试保留 NOT_RUN。 |

SM05 的平面/坡面样本可由 T356 定义直接生成，不依赖真实沉降。若要核 bh/mh 的置换经 prepare 保持 S_D/pb，再在既有 ETS sealed fixture 路径生成同总库存的合法输入并提交。同层透明/线性色域取证可在 host 测试编译开关下暴露 `DrawLayers` 到受控 FBO，沿真实 program/uniform/texture/viewport 执行，跳过背景和前景时标记 `LAYER_ONLY`。若现有 host test seam 无法表达透明或线性目标，developer 只补 test-only 目标和图层选择接口。两个正常整帧对照始终走原 `SingleSandPass::Draw`，不得把重写 GLSL 的结果当实际生产像素。

每案预先冻结 fixture/ROI、FBO 格式、坐标原点和反控。记录原始 RGBA、W/H/stride/row order、实际 typed bytes 和 SHA、layer 模式、GL 版本/renderer、高精度、attachment encoding、samples、dither/blend/write-control 和 GL error。输出原始 RGBA 或无损 PNG，测量基于原始像素尺寸。独立 oracle 先验证对已知错色、漏 EOTF、错误深度/尺度/遮挡足够敏感。不同格式分别建立量化和读取误差包络。样本不足、遮挡、无法判别、原错误逐项记录。只有确认格式和控制色块后才做通道容差，禁止拿固定宽容差覆盖结构失败。

## 3. 颜色链分层与真机最小探针

| 层 | 采集 | 验证范围 |
|---|---|---|
| Host 线性控制 | 明确建 `GL_RGBA8`、查 `GL_LINEAR`，用实际 shader 的受控光学输入和灰阶 0/128/145/168/111/255，含半透明前景的 premultiplied over。记录 `readBuffer=GL_COLOR_ATTACHMENT0`、RGBA bytes、状态。 | 直接比较量化后的线性码。EOTF 应恰好应用于有编码的输入颜色。premultiplied 混合在经声明的线性显示域执行。 |
| Host sRGB candidate | 读同 fixture 的实际 `GL_SRGB8_ALPHA8` 成功提交 FBO，查 `GL_SRGB`。普通出帧与透明层控制分开。 | 证实该 host driver 上的存储编码、混合及 shader 像素。pbuffer 默认样本单列，仅为 host present 链观测。 |
| 真机 XComponent 默认 back | 与 candidate 同一请求，调用 `present(targets.candidate)` 后、`egl.Swap(p.frame)` 前分别冷读 candidate 与 `framebuffer=0` 的同 ROI，记 `PRE_SWAP_CANDIDATE` / `PRE_SWAP_DEFAULT_BACK`，每处记录实际 readBuffer/encoding。 | 核设备驱动上 `sRGB texture decode → 线性 pass-through → 真实 EGL sRGB back encode`。必须取得默认 `GL_BACK`，禁止用 pbuffer 或 candidate 代替。 |
| 真机交换后呈现 | 同一请求的 `Swap` 与 `Unbind` 回执、current/actual token、窗口/上下文 epoch 对齐。为观察系统合成，锁定同一静态 fixture，取无损窗口截图并登记裁剪、屏幕缩放与色彩管理。 | pre-swap back 读数定位 EGL 写入，CURRENT 和实际 swap 佐证该 buffer 提交。系统截图在映射与色彩可核时才作逐码比较，否则单列呈现或观感。截图不会替代 back bytes。 |

现有 `composition_probe.cpp` 提供分离目标、pre-swap 读取和 `captureProbe` 的参考。该 probe 属另一条路径，输出不能代 T364 的光学和 token。给 developer 的最小改动在 `single_sand_pass.cpp`、`single_sand_napi.inc` 及隔离诊断入口。复用原 owner 线程/会话和 `DiagnosticColdFiles` 归档。诊断包显式单次 arm，按预注册 fixture/ROI 及 session/surface/context/geometry/requested token 捕获。常态帧零读回。帧内只将预分配 RGBA 填入内存，待 swap/unbind 收据齐后异步文件输出原 bytes、manifest 和 SHA。故障样本留原 GL/EGL 错、目标和阶段，采集失败单列 `CAPTURE_FAILED`，不能作为成功像素，也不能污染生产 renderer 的 first failure 或 LastGood。保存/恢复 READ_FRAMEBUFFER、read buffer、PACK alignment/rowLength/skip 和 PIXEL_PACK_BUFFER 等受影响状态，失败同样恢复。捕获同步成本单列，性能卡排除相邻帧。

每次上限建议两目标各至多3个64×64 ROI（RGBA 总≤98,304 B）。fixture 固定盒内中性床、AIR/CONTACT_WAIT、背景和前景边界配准，另取一组真实非零 gains。校准灰阶和混合只在标记的测试夹具上切换，沿同 present shader，并独立记录校准 config 和实际目标，禁止放宽生产 palette validator。取证最少两次成功业务画面（中性和非零 gains）加一轮三层目标颜色校准。每份原档记录设备/包 SHA、shader/config/R8/profile/pb/particle hashes、ROI、实际 target/GL 状态、完整 requested/actual token、swap 状态及 SHA。真机无需重做 host 全尺度矩阵，若默认 back 的对照失配才在设备上追加对应最小例。

## 4. 派工与判据

| 责任 | 实际前置和交付 |
|---|---|
| tester 离线像素 | 可立即开工。交 §2/§3 host 原始像素和独立结果，按 SM 子项保留已测、FAIL 与 NOT_RUN。若透明/线性 seam 缺失，指出具体接口，先完成可直接从已提交 FBO 读回的用例。 |
| developer 窗口诊断 | 可并行完成隔离探针、编译和签包。正常入口无读回，维持封签、严格物理及渲染事务。用非机检查 ROI 上限、状态恢复、same-token、失败路径和冷档可取，真机另卡。 |
| tester 真机窗口 | 等可取档诊断签包；包齐即进入真机串行队列。真机验证 candidate/default back/交换证据，若需系统呈现再取无损窗口样本。实测命中相同 fixture、ROI、target 与 token，三个层次分别判定。 |
| effector | 真实材质体验按既有动态与观感任务单独评估。静态像素采集可先做，手持与沉降仍受各自前置约束。 |

像素硬判据是同一 material/config/输入时目标域、mask、alpha、位型稳定与身份正确，正反控能区分各错因。CR/RMS/D及读回容差需按 T356 定标。只要设备颜色链某层出现失配或证据不可用，保留该层 FAIL/NOT_RUN，原档交 architect 首分流。shader/profile/渲染接口归工程，公式效果冲突交 designer，观测方法争议交 manager。前景背景统一 XComponent+shader，错误显示 FAULT 或完整旧 token STALE。T370 工程分流与取证合同已完成，设备、像素与观感的产品结果沿各自后续卡取得。
