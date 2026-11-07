# P-013 P0 主体骨架开工级预设计 v1

日期：2026-10-24。责任：architect。优先级：P5（5）。任务：6adc4c91…dcd5（从原双小活第②项拆出）。

状态：**DESIGN_ONLY / PENDING_MANAGER_ACCEPTANCE / IMPLEMENTATION_NOT_AUTHORIZED**。

本稿只提交设计，不代表 P0 主体获准开工。developer 卡 6adc4394…2edd 的门序仍为 **manager 验收本稿 AND effector 确认 P0a 真机通过**；P0a 卡 6adc4394…609d 排在 s136 验证 6adc5a56…3631 后，预设计不等待真机。不修改冻结锚点、物理算法、测试判线或其它角色账本。本次离线核查源码与本地 SDK，未编译、未运行设备、未写产品或测试代码。

## 1. 输入、适用域与本稿裁定

输入（以当前文件最新修订为准）：

- [立项书 v1.1](proposal-P013-incorporation-v1.md) §2.3–2.6：P0 为独立光场诊断，完整材质合成归 P1。
- [designer 效果目标](design-P013-E2-effect-target-acceptance-v1.md)：P0 近似坡度不得参加 σ=6 精确等价判定；候选阈值没有冻结。
- [换栈评估](architecture-P012-E2-route3-xcomponent-egl-assessment-v1.md)：仅沿用集成边界；早期七字段/row7、跨层混合描述由立项书 v1.1 覆盖。
- P0a `aa26a36`、[工具链四坑记录](p013-p0a-toolchain-notes-v1.md)、[P0a 回执](p013-p0a-receipt-v1.md)。当前 `hello/hello_surface.cpp` 是迁移源，构建交付不等于真机验收。
- `P012WaterSandRenderer.ets` 的 `buildDisplayProfiles/heightToY/e2EnsureLightMap`、`P012MaterialSampler.ets` 的 split 插值、`SandCanvas.ets` 帧分发与 seg3。
- [seg3 报告最新 v2](fps-seg3-P012-v0.md)、[s136 归因](p012-s136-closure-receipt-v1.md)、`priority.md` 15:15 排期。旧 s133 dyn 安静尾部与 s134 回归归因已被更正，不据旧表承诺收益；bcada09 的真机效果仍待复测。

工程裁定：

1. P0 保留完整 Canvas，GLES 只输出沙区 Lmat 的可逆诊断灰阶。没有 B/C 纹理，没有读取下层 Canvas，没有 `DST_COLOR` 跨层乘法，没有 ToneMap 材质合成。P0 不签 M1/M2。
2. 同一 UI 帧回调同步提交；不引入渲染 worker、异步队列或跨帧待消费 TypedArray。GL/EGL current 只在 NAPI owner 线程的调用栈中存在。
3. T_data 四通道固定为 **sandTop/waterTop/bh/mixStart**，128×1 RGBA32F，材料高度单位；不混用归一化高度、显示 y 或七行布局。
4. shader 取样与现有连续 sampler 对齐，使用 `texelFetch+mix`，不依赖 RGBA32F 线性过滤扩展。P0 raw 差分的 raw 是“未做 E2 σ 宽面低通”，不是取消既有显示 profiles 的四遍三点平均。
5. 立项书 shader 草案的 `vec4(1)` 非沙输出会遮白；RGBA8888 直接输出 Lmat>1 会饱和；y 翻转注释与实际坐标也不充分。本稿用透明非沙输出、显式坐标逆映射、固定诊断编码替代这些草案细节，不改变 Lmat 公式。

## 2. 模块、文件与责任边界（以下均为待授权实施清单）

| 文件 | 职责及约束 |
|---|---|
| `entry/src/main/cpp/p013renderer/napi_bridge.cpp/.h` | 参数解析、单实例 session、XComponent 回调注册、owner 校验、帧事务、错误结果；不放着色公式 |
| `p013renderer/egl_context.cpp/.h` | 从 hello 搬 EGL 链；surface 世代、current、重建、销毁与幂等回滚 |
| `p013renderer/shader_pass.cpp/.h` | 编译链接、uniform location 缓存、固定全屏三角形、诊断 draw、GPU 查询可选槽 |
| `p013renderer/tdata_uploader.cpp/.h` | 一张不可变存储纹理、2048B staging、版本校验、上传及资源计数 |
| `p013renderer/render_contract.h` | POD 参数、枚举、版本常量、FrameResult/Telemetry；四模块共同头 |
| `p013renderer/glsl/e2_lmat.vert/.frag` | GLES 3.0 ES 源码；构建时嵌入头，运行时不依赖工作目录读文件 |
| `entry/src/main/ets/render/P013RenderTypes.ets` | 显式 ArkTS 类/接口及 so 类型声明；禁止 any/unknown |
| `entry/src/main/ets/render/P013FrameAdapter.ets` | 只读快照→复用 512-float 缓冲；同源显示 profiles；容器单位/旋转映射 |
| `entry/src/main/ets/render/P013RenderController.ets` | 模式切换、单次 postFrameCallback 链、错误移除诊断层、遥测关联 |
| `entry/src/main/ets/pages/P013HelloSurface.ets` | 保持 hello 独立入口与旧 API；后续修正自身隐藏/销毁帧回调门控，不冒充主线驱动 |
| `entry/src/main/ets/pages/SandCanvas.ets` | 仅接诊断开关/层与 Canvas render 后的提交点；不改物理、手势和旧 render 分支语义 |
| `entry/src/main/cpp/CMakeLists.txt` | 保留 `p013hello`，新增 `p013renderer` 目标；共享 egl_context 源码分别编入两个 so，不共享运行时单例 |

hello 的 Hook/注册/清屏逻辑保留薄壳，EGL 初始化/清理搬到公共实现，不保留两份有分叉的 EGL 链。两个诊断页互斥挂载，禁止同时跑两条帧驱动。沿用 C++17 和现有五个库实名：`libace_napi.z.so/libace_ndk.z.so/libEGL.so/libGLESv3.so/libhilog_ndk.z.so`。不恢复错误的 `libace_napi_arkui`。使用 `OH_NATIVE_XCOMPONENT_OBJ`、uint64 尺寸与 `reinterpret_cast<EGLNativeWindowType>`，详见四坑路标。shader 嵌入的 hash 必须来自实际打包源码，不能只 hash 文档草案。

## 3. napi_bridge：完整帧契约

### 3.1 外部 API（新 so，不改变 hello 旧签名）

```ts
// 全部同步返回；ArkTS 用显式接口/类实现下列契约。
interface P013NativeRenderer {
  init(componentId: string): P013ControlResult;
  getState(): P013RendererState;
  setActive(active: boolean): P013ControlResult;
  drawFrame(frame: P013FrameInput, tdata: Float32Array,
    params: P013LightParams): P013FrameResult;
  release(): P013ControlResult;
  drainTelemetry(): P013TelemetryBatch;
}
```

`init` 绑定已由 XComponent libraryname=`p013renderer` 回调注册的 ID，**不接收字符串转指针或 ArkTS window handle**。若早于 Created 返回 NOT_READY，页面在 getState 显示 WINDOW_AVAILABLE 后重试，不每帧反复 init。P0 仅一个活跃 session；重复同 ID init 幂等，第二 ID 返回 SESSION_BUSY。release 后重新 init 分配新 sessionId；退出后 owner 重建才允许换线程。sessionId 为进程内递增安全整数，所有计数禁止超 JS 安全整数。

`P013ControlResult={code,sessionId,state,surfaceEpoch,contextEpoch}`；`P013RendererState` 另含 `widthPx,heightPx,active,ownerBound,lastErrorCode`。尺寸是 native 实测像素，不信页面 vp 值。init 仅绑定 NAPI owner、建 CPU 状态，不在 surface 回调创建 GL。

### 3.2 FrameInput / LightParams（所有字段必须显式提供）

| FrameInput 字段 | 类型、单位、校验与用途 |
|---|---|
| `sessionId, frameId` | 安全非负整数；frameId 在 session 内严格递增，从 1 起；每次 draw 尝试消耗一次 ID，失败也不得复用 |
| `snapshotId` | 安全非负整数，物理快照 revision；dormant 可复用；不是 frameId |
| `surfaceEpoch` | getState 返回的世代，必须等于当前值，防 resize/重建后旧几何提交 |
| `fieldEpoch,geometryEpoch,continuityEpoch` | 安全非负整数，沿用页面来源；变化帧必全量参数/数据上传，无旧 GPU 历史复用 |
| `appearanceConfigRevision,lightConfigRevision` | 安全非负整数，只读现有配置版本及诊断候选版本；无第二外观持久化通道 |
| `frameTimeNs` | vsync 回调时间戳，有限非负 number，仅作 ArkTS 时序身份；不与 native 单调钟直接相减 |
| `surfaceWidthPx,surfaceHeightPx` | 正整数，须等于 eglQuerySurface 结果；不匹配返回 STALE_SURFACE |
| `boxXPx,boxYPx,boxWidthPx,boxHeightPx` | 容器在 surface 内的左上原生 px；宽高>0、矩形在 surface 内；由 Canvas 同一布局乘实际 surface/Canvas 比率转换 |
| `capacity` | 有限正数，材料高度总尺度，与 Canvas heightToY 同源 |
| `visualInverted` | boolean；只反向像素坐标，不预反转纹理、不再给 XComponent 做页面 rotate |
| `diagnosticEnabled` | boolean；false 为透明清屏并 swap，不能只是跳过 draw 留旧热图 |
| `T_dataLayoutVersion,slopeKernelVersion,zOriginVersion` | 精确等于 `1,"E2_APPROX_P0",1`；不支持版本返回 VERSION_MISMATCH |

`P013LightParams={la,ld,lmin,lmax,lref,ellLightMaterial,lightX,lightY,lightZ,sigmaCandidate}`：全为有限浮点；`ld>=0, 0<=lmin<=lmax, ellLightMaterial>0`；光向模长>0，桥侧归一化，记录归一化结果；`lref` 必须位于 `[lmin,lmax]`。sigmaCandidate 为非负元数据候选（常见 6），**不传入 shader 运算**。ell 候选 24 材料单位转换为 `ellLightPx=ellLightMaterial*boxHeightPx/capacity`，zOriginVersion=1 的 shader 深度仍是显示 px。

不在接口里冻结 La/Ld/光向默认数值；developer 必须原样读取已登记的候选配置，随证据保存完整参数。为了做平沙≈1反例，诊断 fixture 明确使用 `lref=clamp(la+ld*max(0,lightZ),lmin,lmax)`（light 已归一化）。若候选配置另选 lref，则平台期望是公式值，不能擅自调整 lref 强行过线；报 designer 复核断言适用域。迎/背光围绕 1 的符号也须按实际坡度、光向、截断和 lref 计算，不承诺任意输入都异号。

### 3.3 帧事务与所有权

`drawFrame` 内固定次序：检查 NAPI 参数类型/长度/byteOffset → 校验 owner/session/frameId → 记录此次 attempted ID → 校验 epoch、参数和512个值 → 锁定 surface 元数据与借用 window → 必要时建 EGL/资源 → 上传 T_data → 提交全套 uniforms → clear/draw → swap → unbind → 返回结果。验证失败不更改上一套已提交配置，不上传半包；仅序号/错误遥测可变。资源或 GL 失败后不把本帧记作成功。

`napi_get_typedarray_info` 只接受 float32、长度512、有效非脱离 buffer；使用返回 data 指针（已含 offset），不得再叠 byteOffset。不缓存指针/JS 对象、不启动异步消费。同步复制到固定 `std::array<float,512>` 后校验，传给同步 `glTexSubImage2D`；返回后 JS 可立即复用原缓冲。所谓零拷贝仅指 NAPI 取得视图不另分配 ArrayBuffer，**不表示 GL 零复制或 JS buffer 可跨帧借用**。T_data 与 frame/params 是一个原子帧，取消草案 feedTData/setParams 分三次调用的半新半旧风险。

`P013FrameResult={code,sessionId,frameId,snapshotId,surfaceEpoch,contextEpoch,uploadedFrameId,submittedFrameId,swapSucceeded,lastSwapFrameId,state,eglError,glError,recoveryRequired}`。无对应成功帧 ID 时为0；code=OK 仅当上传、draw 与 swap 全成功，`lastSwapFrameId` 才更新。swapSucceeded 不等于已屏幕呈现，不能命名 presentedFrameId。OFF 清屏帧 uploadedFrameId=0、submittedFrameId=frameId（透明清屏提交），swap 仍检查。

### 3.4 错误码与处理

| code | 值 | 行为 |
|---|---:|---|
| OK | 0 | 本帧 swap 返回 EGL_TRUE |
| NOT_READY / INACTIVE | 1 / 2 | 没 window 或后台；不做 GL，不计成功 |
| INVALID_ARGUMENT / VERSION_MISMATCH | 100 / 101 | 类型/长度/非有限值/层序/合法域不符；保留 CPU 上一成功状态，移除可见诊断层防旧帧误认 |
| STALE_FRAME / STALE_SURFACE | 102 / 103 | 序号非递增或 epoch/尺寸不匹配；废弃本帧，下个新 ID 重取状态与几何 |
| WRONG_THREAD / SESSION_BUSY / RELEASED | 104 / 105 / 106 | 拒绝；不迁 current，不另起后台补提交 |
| EGL_INIT_FAILED / CONTEXT_LOST / BAD_SURFACE | 200 / 201 / 202 | 进入失效/恢复状态；CPU最新包可保留，GPU名全部失效；页面立刻隐藏 GLES 层 |
| SHADER_COMPILE_FAILED / GL_RESOURCE_FAILED / GL_DRAW_FAILED / SWAP_FAILED | 300 / 301 / 302 / 303 | 保存实际错误与编译日志 hash；停止诊断臂，Canvas 继续；swap 失败绝不返回 OK |

控制接口使用相同适用错误码。常规错误返回结构，不抛异常穿过帧驱动；不可恢复 NAPI 创建结果失败由桥返回 JS Error，controller 捕获并切回 Canvas。GL 检查每阶段一次（upload/draw），EGL 在失败调用后立即取错，保留 stage，禁止后续调用清掉根因。诊断结果单列错误计数，不能只在统计时剔除失败帧。

## 4. egl_context：生命周期与线程协议

### 4.1 C++ 接口

```cpp
Status OnWindowCreated(OH_NativeXComponent*, NativeWindow*);
Status OnWindowChanged(OH_NativeXComponent*, NativeWindow*);
void OnWindowDestroyed(OH_NativeXComponent*, NativeWindow*);
Status EnsureCurrent(const SurfaceKey&, SurfaceInfo&); // 仅 NAPI owner
Status Swap(uint64_t frameId);                        // 仅 NAPI owner
Status Unbind();                                     // 每次帧调用 finally
void DestroyUnbound(ResetReason);                    // 锁内，无 current
Status SetActive(bool);                              // 仅 NAPI owner
void Release();                                      // 幂等
```

内部持有 display/config/context/surface、window借用指针、width/height、surfaceEpoch/contextEpoch、状态及资源注册表。`OnWindow*` 通过同一 session mutex 与帧互斥；不调用 JS/NAPI，不假定系统回调线程等于 UI 线程。本地 SDK 回调声明未给出这种同线程保证。window 仅在 Created–Destroyed 有效期内借用，不对借用指针擅自销毁 native window。

每帧/控制调用在 owner 完成 unbind 后才解锁；Destroyed 取得锁意味着无 current，然后销毁 EGL surface/context/display（context 销毁回收本代 GPU对象），清空借用指针。Destroyed 不延后持有裸 window 到下个 UI 帧。资源正常 release 时若可 makeCurrent 则 glDelete；若 current 不可用则随 context 销毁回收，计数登记 abandon，不在错误线程 glDelete。锁内不得回调 UI；回调不得等待 UI 工作。**单线程同步模型仍有 swap 持锁期间 Destroyed 等待的残余风险**：必须记录 lockWaitMs/swapMs，真机生命周期验证若发生平台锁反转，立即报 architect 修订生命周期协议，developer 不自行加工作线程或用悬空 window 绕过。

### 4.2 状态与世代

| 事件 | 状态转移与资源动作 |
|---|---|
| init，无 surface | WAIT_WINDOW；CPU对象准备，GL对象数0 |
| Created | 比对 componentId、发布 window/尺寸，surfaceEpoch++，WINDOW_AVAILABLE；重复同 window 无新代不重复创建 |
| 首个有效 draw | EGL getDisplay→initialize→choose RGBA8888/WINDOW/ES3→bind ES API→createContext→createWindowSurface→makeCurrent→query尺寸；资源全建成功后 READY，contextEpoch++ |
| Changed | 锁内重查 native 尺寸，surfaceEpoch++；下一 draw querySurface 确认并 viewport 更新，旧尺寸帧拒绝；零尺寸进 SUSPENDED |
| setActive(false)/页面隐藏 | 停止新帧链；锁内拆 EGL/GPU、保留仍有效 window 与CPU包，SUSPENDED；不等 Destroyed 才停止 swap |
| setActive(true)/页面显示 | 有 window→WINDOW_AVAILABLE，下个新 ID 完整建资源/重传；否则 WAIT_WINDOW；只注册一条回调链 |
| Destroyed | 与 draw 互斥，清理并清 window，surfaceEpoch++，WAIT_WINDOW；晚到旧 window 的 Changed/Destroyed 只记 staleLifecycleCount，不破坏新 window |
| EGL_CONTEXT_LOST | 标失效，GPU名清零，DestroyUnbound；window 未销毁则下一有效帧可重建一次；不重用失效 texture/program |
| BAD_NATIVE_WINDOW/BAD_SURFACE | 等新的 Created/Changed 确认后再建，不无限复用坏指针重试 |
| release/页面销毁/env清理 | RELEASED，移除 owner/帧回调引用，CPU/GPU计数归零；晚到 native 回调安全空操作；callback storage 活到 XComponent 回调解绑/组件销毁完成 |

初始化每步失败按逆序回滚已有句柄，全部恢复 EGL_NO_*，重复 Release 不重复 terminate。makeCurrent 失败也必须执行 finally 清理；解绑失败则会话停止，禁止在其它线程继续使用仍 current 的 context。连续恢复失败不热循环：每个新 surfaceEpoch 自动尝试最多一次，同代 context loss 最多恢复一次，再失败需显式诊断重启，Canvas 始终可用。

相较 hello 必修三点：`DrawFrame` 不再把 g_ready 当 swap 成功；不在 Created 假定 current 永远留在以后 draw 所在线程；页面隐藏和销毁禁止继续自注册 postFrameCallback。hello独立页后续如使用新生命周期也须保持清屏验收含义。

## 5. tdata_uploader：布局、数据源、内存与上传

### 5.1 布局 v1（唯一 P0 契约）

`T_data[4*c+0..3]=(sandTop,waterTop,bh,mixStart)`，c=0..127，mixStart=bh+bl；总512 float32=2048B，无 header、无坡度/悬沙行。全部为未归一化材料高度。每列有限且 `0<=bh<=mixStart<=sandTop<=waterTop<=capacity`；若有效物理场违反最后关系须上报 designer 复核真实语义，不能静默排序/夹值通过。Float64转Float32后仍检查层序。capacity以帧参数提供。

适配器只读同一 `WaterSandSnapshot`；先按现有 buildDisplayProfiles 的四遍三点平均（边缘复制、每遍 scratch→set）产生四条曲线，再打包，与连续 sampler 同源。可新增只读 `copyDisplayProfilesTo(out512)` 窄接口来复用当帧 renderer 已算的数组，不外露可变数组；实现若选独立适配器则逐值复现同式并验证同输入一致，**不能反复跑 e2EnsureLightMap 的128×256烘焙来取四列场**。此选择不改 Canvas 现有平滑和物理快照。此处“复用路线②生成器”是复用数据来源/公式，原型的 zMid/capacity 偏移、LUT与宽面低通不直接作为P0数值参考。

### 5.2 内部 API 与 GL 状态

```cpp
Status TDataUploader::Create(uint64_t contextEpoch);
Status TDataUploader::Upload(const float* rgba512, uint64_t frameId);
GLuint TDataUploader::Texture() const;
void TDataUploader::Reset(bool contextCurrent);
Status ShaderPass::Create(const ShaderSources&, uint64_t contextEpoch);
Status ShaderPass::Draw(const FrameUniforms&, GLuint tdataTexture);
void ShaderPass::Reset(bool contextCurrent);
```

Create：glGenTextures一次→GL_TEXTURE_2D→`glTexStorage2D(1,GL_RGBA32F,128,1)`；MIN/MAG=NEAREST，WRAP_S/T=CLAMP_TO_EDGE，base/max level=0。每成功 ON 帧上传一次 `glTexSubImage2D(0,0,0,128,1,GL_RGBA,GL_FLOAT,staging.data())`，显式unbind PIXEL_UNPACK_BUFFER、UNPACK_ALIGNMENT=4、rowLength/skipPixels/skipRows=0；不走 PBO、不 glTexImage 重分配、不 mipmap、不 glFinish。context恢复/epochs变化首帧完整上传；P0不以dirty阈值跳过，dormant帧仍可作为60Hz上传压力用例。OFF帧不上传，只透明清屏。

纹理固定单元0，`glActiveTexture(GL_TEXTURE0)`，bind texture，`u_tdata=0`。RGBA32F只采样不作render target，不要求float framebuffer或float线性过滤扩展。program/VAO/location在Create分配，Draw禁止compile/link/glGetUniformLocation/新建纹理。

固定内存：JS输出2048B，适配器4条Float64 profile+1条scratch共5120B，native staging2048B，纹理payload2048B；驱动分配与Surface buffer不计入payload下界，单列实测。诊断遥测环固定2048条；覆盖旧条时必须累计 droppedTelemetry，不能伪装完整采样。无逐帧vector增长、JS引用累积或PNG读回。

## 6. shader_pass：坐标、mask与诊断编码

### 6.1 坐标与 uniform 绑定

shader `#version 300 es`、highp float/int；顶点用gl_VertexID生成全屏三角形（空VAO一次创建），fragment用gl_FragCoord；不用未定义方向的v_uv推导材料高度。surface原生左上坐标 `p=(gl_FragCoord.x, surfaceH-gl_FragCoord.y)`；容器局部 `q=p-box.xy`。若 visualInverted，先 `q=box.zw-q`（围绕同一容器中心180°）；texture列不重排、T_data高度不翻转、光向在该材料显示基坐标中定义，随容器一起翻转。记录 `lightFrame=CONTAINER_PRE_INVERT_V1`，避免误称屏幕固定顶光；屏幕固定光向若后续需要由designer另定revision。

容器外透明；内部 `h=(1-q.y/boxH)*capacity`，`pxPerMaterial=boxH/capacity`。列参数 `r=clamp(q.x/(boxW/128),0,127)`，`i=min(floor(r),126),t=r-i`；最右段恒取127。`D(q.x)=mix(texelFetch(T_data,(i,0)),texelFetch(T_data,(i+1,0)),t)`，因此无硬件半texel偏移，与sampler的x/colW契约一致。全量 float32误差与像素边界量化由后续测量报告，不用宽松阈值吞掉方向错误。

mask（物理高度向上，显示y向下）：

- 沙：`0<=h && h<S`；h=S归非沙。H: `h<bh`；L: `bh<=h<mixStart`；M: `mixStart<=h<S`，仅供诊断分类，不上传材质颜色。
- 水/雾几何包络：`S<=h && h<waterTop`，透明；h>=waterTop也透明。四标量不能重建suspH/suspL的浓度mask，P0明确不声称精确重建雾密度或界面光学带。
- 连续sampler mask与Canvas逐列fillRect轮廓在列间可有实现差异；P0 mask对照对象是同profiles的连续sampler，不声称和旧逐列轮廓任意形状均±1px。若正式用例要求与旧轮廓±1px而不相容，交designer/manager处理，不能改线或用另套平滑遮掩。

| uniform | 来源 |
|---|---|
| `u_tdata` | texture unit0 |
| `u_resolution` | eglQuerySurface实测width/height |
| `u_box` | (boxX,boxY,boxW,boxH)原生px |
| `u_capacity,u_visualInverted` | 帧契约 |
| `u_lightDir` | 桥校验后归一化vec3 |
| `u_la,u_ld,u_lmin,u_lmax,u_lref` | 完整候选参数包，每帧绑定避免半包 |
| `u_ellLightPx` | ellLightMaterial×pxPerMaterial |

没有u_sigma运算槽，没有T_tex/T_param纹理；P0参数使用uniform。所有必需location缺失（被错误优化掉或名字不符）在Create失败，不静默跳过。

### 6.2 GLSL 运算草案（设计片段，非已实现shader）

```glsl
// D(x) 按 §6.1 显式 texelFetch/mix，已先做容器范围与旋转处理。
vec4 d = D(q.x);
float h = (1.0-q.y/u_box.w)*u_capacity;
if (h < 0.0 || h >= d.r) { outColor=vec4(0.0); return; }
float colW = u_box.z/128.0;
float dx = colW;
float pb = (D(q.x+dx).r-D(q.x-dx).r)
           * (u_box.w/u_capacity)/(2.0*dx);
vec3 n = normalize(vec3(-pb,0.0,1.0));
float lf = clamp(u_la+u_ld*max(0.0,dot(n,u_lightDir)),u_lmin,u_lmax);
float zPx = (d.r-h)*(u_box.w/u_capacity);
float lm = 1.0+exp(-zPx/u_ellLightPx)*(lf-u_lref);
// 固定编码 DIAG_GRAY_V1：lm=1→0.5，lm=0→0，lm=2→1。
float encoded = clamp(0.5*lm,0.0,1.0);
outColor = vec4(vec3(encoded),1.0);
```

pb两端D自行clamp，固定2dx分母，与端点复制中心差分一致。它是显示px/px的近似坡度，未做额外σ核，不与原型“材料单位/列”的坡度强行数值等同。zPx以本列沙面为0，绝不从capacity减bandCenter。衰减单调指 `abs(Lmat-1)` 随深度不增（背光Lmat本身随z上升），不得要求所有Lmat均随z下降。

诊断编码固定，不按当帧min/max自动拉伸；图例灰度0.5为中性。参数理论 `Lmat` 范围为 `[min(1,1+lmin-lref),max(1,1+lmax-lref)]`；正常诊断要求处于[0,2]，否则参数拒绝并说明DIAG_RANGE，不能悄悄clip后宣称光场正确。离线CPU可从无损目标值还原 `Lmat=2*encoded`，但设备色彩变换/8bit量化后的屏幕灰度不能无校验直接当精确浮点值；同场同时保留T_data/参数/CPU公式参考。

每帧先关闭blend/depth/stencil/scissor/cull，设置viewport与colorMask，clear=(0,0,0,0)，ON再draw。沙像素alpha1，非沙alpha0，符合premultiplied alpha的0/1特例；OFF只clear并swap。诊断层覆盖沙区显示编码，不能拿它测最终材质Y/暗面纹理。颜色链记录EGL配置及实际截图格式，P1色彩/ToneMap冻结仍单独前置。

## 7. 双轨共存、帧时序与异常降级

显式模式：`CANVAS_BASELINE`（默认、不挂载XComponent）；`CANVAS_TOPLIGHT_REFERENCE`（旧toplight对照，GLES不挂载）；`GLES_LMAT_DIAG_OFF`（Canvas toplight OFF、透明surface清屏）；`GLES_LMAT_DIAG_ON`（Canvas toplight OFF、沙区诊断编码）。不能用一个布尔值同时开两个toplight。canonicalEvidence/物理快照证据路径强制基线，不把诊断层带入既有E3/B/C冻结采集。

```text
同一 postFrameCallback(frameTimeNs)
  → 既有物理 step / copyStateTo（零改动）
  → 既有完整 Canvas render / profiles（Canvas renderMs原边界保留）
  → P013 adapter 拷贝/打包同一 snapshot（若复用profiles必须核对revision）
  → native drawFrame：校验→EGL current/重建→upload→uniform→draw→swap→unbind
  → 写固定遥测环；下一次vsync仅注册一次
```

独立hello页不与主页面共享ticker；页面隐藏设置controller.active=false，已有回调检查active/owner后直接return且不再注册，显示时只启动一次。无surface或零尺寸时仍保留Canvas原驱动，不每帧重建EGL。帧包字段不足或参数非法时不显示旧热图：controller移除/隐藏诊断层，并记录故障帧。正常ON→OFF先提交透明帧再隐藏；swap失败时无法保证清屏，必须直接移除XComponent，不能靠透明clear“应该成功”降级。

GL不可用、shader失败、上传失败、生命周期恢复超限统一回到Canvas基线；旧Canvas可用性不依赖native返回。失败事件不改物理状态/seed/appearance、不扩大重烘、不自动降低物理频率。隐藏层再次启用须新surface/session或明确重试并重传全部资源，不把自动降级窗口混入成功ON统计。

P0不停止任何Canvas材质层；P1未来切换点是Canvas render分发内的沙区层责任转移，须完整GLES材质pass和双轨验收后另卡实施，不能本轮为了性能先删Canvas沙体。P2水/雾迁移、P3退役均不被此稿授权。

透明surface合成与输入透传仍是设备能力验证项：P0层设置HitTestMode.None、按钮/手势仍归原Stack；ON/OFF同位置保留水/雾ROI。若系统合成忽略alpha（透明变黑/白），不把非沙画白掩盖：暂停overlay，记录ALPHA_COMPOSITION_UNPROVEN；可在独立诊断页继续离线式光场取证，但不能作为“水雾逐像素不变”的PASS。

## 8. 帧时风险与遥测字段登记

### 8.1 时钟、字段与采样

native阶段统一 `CLOCK_MONOTONIC` 纳秒差换ms（双精度），不用Date.now测0.1ms。ArkTS打包/跨桥采用本地SDK支持的单调高精度时钟，实施前确认精度并登记 `arkClockName/arkClockResolutionNs`；不能以毫秒Date.now产生的0冒充<0.1ms。跨两时钟只比较持续时间，不直接相减绝对时间。每条native记录都有frameId/sessionId/snapshotId/surfaceEpoch/contextEpoch/模式/三版本号；一次run头另含物理快照hash、renderSeed、配置revision、完整lightConfig、rendererCommit、shaderHash、T_dataHash、包hash、设备/DPI/温度、colorSpace/maskRevision/captureFormat、buildDirty及sourceTreeHash。脏包必须明确来源，不以HEAD代表实际HAP。

| 模块 | 字段名 | 采样口径、边界 |
|---|---|---|
| adapter | `p013.packMs,profileCopyMs,profileBuildMs,packedBytes,snapshotId` | 每ON帧；profiles重算与只拷贝分别计；固定packedBytes=2048；不混入旧Canvas renderMs |
| bridge | `p013.bridgeValidateMs,bridgeCopyMs,bridgeCpuMs,drawCallMs,ownerThreadId,frameGap,code` | bridgeCpuMs从native入口到返回，drawCallMs为ArkTS调用前后含跨桥结果创建；每次尝试含失败 |
| EGL | `p013.eglInitMs,makeCurrentMs,unbindMs,swapMs,lifecycleLockWaitMs,eglError,errorStage,state` | 每调用/每事件；swapMs是CPU阻塞，不是GPU时长或RS合成 |
| EGL资源 | `surfaceCreateCount,surfaceDestroyCount,contextCreateCount,contextDestroyCount,contextLossCount,recoveryAttemptCount,staleLifecycleCount` | 事件累计，附世代；后台/恢复段单列 |
| uploader | `p013.uploadCpuMs,uploadPathMs,uploadBytes,uploadCount,textureAllocCount,textureLiveCount,texturePayloadBytes,glError` | uploadCpuMs=bind/像素状态设置至subImage返回含错误检查；uploadPathMs=桥typedarray读取+copy+校验+upload的阶段和；不含shader/swap/EGL冷建 |
| shader | `p013.shaderBuildMs,shaderHash,uniformCpuMs,drawCpuMs,gpuPassMs,gpuTimerValid,gpuDisjoint,diagEncoding` | build仅冷建；drawCpu为uniform后draw提交；GPU项仅异步有效query结果，回填原frameId |
| 固定资源 | `programLiveCount,vaoLiveCount,nativeOwnedBytes,telemetryLiveCount,droppedTelemetry,gpuQueryPendingCount` | 每秒快照，峰值与release后值另记；nativeOwnedBytes不包括驱动/系统surface |
| controller | `p013.totalAddedCpuMs,frameIntervalMs,callbackStartIntervalMs,mode,diagnosticVisible,fallbackReason,attemptCount,swapSuccessCount` | 每帧；totalAddedCpuMs=pack+drawCall+本轮控制成本，含swap阻塞；callback间隔不冒充屏幕present |
| 进程/设备采集 | `processRssBytes,nativeHeapBytes,deviceTempC,thermalState,presentedIntervalMs,presentedSource,presentCorrelationValid` | 内存/温度1Hz或工具实际频率；present由系统trace/实际呈现证据，缺失为null+UNAVAILABLE |

日志 `P013_FRAME {schemaVersion:1,...}` 独立于 `WS_FRAME ... seg=`，不改旧payload及解析器。每帧进入预分配环，平时1Hz聚合P50/P95/max/n/failCount，不逐帧hilog；采集模式分批drain，超过2048条未取走必须暴露丢样，完整108000帧链不允许静默覆盖。导出/日志的时间用 `telemetryDrainMs` 单列，性能臂同采样配置，避免“开遥测才掉帧”。

GPU计时若本地扩展/真机支持disjoint timer query，固定小环异步轮询已完成query，禁止等待、glFinish或逐帧glReadPixels；不支持则gpuPassMs=null、gpuTimerValid=false，改用独立trace取证。不得用drawCpuMs<1ms替代GPU pass<1ms。readback仅静态黄金图专用run，记录readbackMs和captureFrameId，排除在性能run之外。

### 8.2 seg3关联与判读

旧 `physMs/renderMs/presentMs` 继续保持原语义；旧presentMs=`max(0,frameDelta-phys-render)`在新增native后会包含部分native成本及帧间错位，不是GPU/RS实测。P013另报 `residualMs=max(0,frameIntervalMs-physMs-canvasRenderMs-totalAddedCpuMs)`，明确RESIDUAL_ESTIMATE；不覆盖旧presentMs，不把残差称合成耗时。分析按同run/frameId配对，各阶段P95不可直接相加当端到端P95。

复测矩阵：同构建/同设备/同温分层的settle、tilt、heavy × Canvas基线、GLES OFF、GLES ON；动态有效窗与dormant尾部分列。保留完整原始分布，旧“physicsTime>20ms”过滤仅作为带标记的历史兼容子集，不能用它删除真实重载退化。汇总n/有效时长/失败率、P50/P95/max、热态与场景身份，脱离真实呈现源只能判CPU提交性能。

### 8.3 风险与待验收指标（均无实测承诺）

| 风险 | 定位字段/验证 | 处置 |
|---|---|---|
| UI同步swap阻塞vsync、双Surface合成增耗 | swapMs/drawCallMs/presentedIntervalMs，配对OFF/ON | Canvas继续；不擅自改异步线程，超预算交architect/manager |
| 小纹理subImage因前帧GPU读而stall | uploadCpuMs/uploadPathMs/max/慢帧frameId | P0先如实测；PBO/多纹理是后续方案修订，不能先宣称小payload必快 |
| fragment量随分辨率增长 | gpuPassMs/box原生px/实际像素数 | 保留原生采集尺度，缩分辨率不得用作原尺度PASS |
| init/compile/恢复峰帧 | eglInitMs/shaderBuildMs/recovery计数 | 冷/暖/恢复分组，不能从主报告隐去 |
| profiles重复计算/桥拷贝/日志放大成本 | profileBuildMs/packMs/bridgeCopyMs/drainMs | 固定缓冲与只读窄导出；不改Canvas平滑公式 |
| buffer/资源长期增长 | nativeOwnedBytes/各liveCount/进程RSS | 所有重建/失败路径计数，异常立即停诊断、保存证据 |
| mask/旋转/alpha错误使非沙被盖住 | frame identity、mask边界图、OFF/ON无损ROI | 不以热图可见替代非沙不变；平台不支持则标BLOCKED |

立项书原验收保留：上传路径单次<0.1ms；诊断pass<1ms；60Hz持续30分钟无增长；ON相对Canvas-only帧时差<1ms且无掉帧回归。具体落数：

- 上传以稳定READY帧的uploadPathMs（NAPI读取/校验/copy+GL上传，2048B）为单次口径，同时报含跨桥drawCallMs与全部max/异常；不能只取P50<0.1ms就写单次达标。冷建帧另列，不混成上传耗时。
- pass同时报uniform+draw CPU与有效gpuPassMs；无GPU测量则此项PENDING，不以链接GLES库等同shader吞吐验证。
- 长稳目标108000次成功ON上传（60×1800），不足60Hz或中断须如实标未完成，不以延长稀疏调用冒充60Hz。固定资源计数稳态不增长、release归零；1Hz内存曲线与baseline配对。驱动/RSS“无增长”容差待tester基于工具重复性预注册，architect不自创可放宽的MB阈值。
- 30min结果是整链长稳，不要求UI每帧刷新状态文本；无屏幕呈现数据时只能说提交频率，不能说60fps呈现或90Hz单拍。相对帧时差的统计/窗口由tester沿原口径确认，不由developer选择最有利尾部。

## 9. 三版本号及后续升级规则

| 登记项 | P0固定值 | 语义与升级触发 |
|---|---|---|
| T_dataLayoutVersion | 1 | 128×1 RGBA32F，R=S/G=W/B=bh/A=mixStart，材料高度；增加坡度行、改变通道/归一化必须新版本 |
| slopeKernelVersion | E2_APPROX_P0 | 已有显示profiles上raw相邻列中心差分、显示px/px、边缘复制；未做σ=6核，禁止精确等价结论 |
| zOriginVersion | 1 | 本列S为z=0，shader z为显示px，ell由材料单位同步换算；若改shader深度单位/原点必须新版本 |

补充证据身份：`profileKernel=DISPLAY_4X3_V1`、`maskRevision=P013_CONTINUOUS_SAND_V1`、`diagEncoding=DIAG_GRAY_V1`、`lightFrame=CONTAINER_PRE_INVERT_V1`。这些是新诊断臂的实现描述，**不替换P-012冻结maskRevision**。P1坡度row/σ核与最终合成由designer/architect另版定义，禁止无版本变更偷换成“七行已实现”。

## 10. developer实施切片与交接检查（授权后执行）

1. 四模块头/错误码/so声明与帧事务先落地，维持hello旧API；构建源表与shader嵌入hash可核验。
2. 搬EGL链及状态机，故障注入覆盖部分初始化失败、swap失败、后台、Changed/Destroyed乱序、context loss、重复release。任何回调线程假设都须有证据；不复用hello常驻current捷径。
3. 固定RGBA32F上传链及同源profiles适配，覆盖非零TypedArray offset、长度错误、NaN、层序、版本、旧frame/epoch；验证坏帧无半更新。
4. shader诊断pass按本文公式和CPU参考成对核对平沙、双坡、边缘、薄/空沙、H/L/M界、深度、光向镜像、180°与非全屏box；保留数据和参数，不靠屏幕肉眼猜数。
5. 接双栈显式模式、透明OFF/失败移除层与单ticker；既有Canvas/物理证据臂回归，不能削旧材质层抵消新增开销。
6. 接固定遥测环与分组导出；把§8字段/边界交tester/effector建立实际采集。资源长稳、GPU计时、alpha/输入/呈现只能在后续真机卡验收，本稿不代跑、不预签。

以上是最小可实施顺序，不替代manager分卡与tester测试先行。预设计完成检查：四件套接口、纹理/参数/序号/错误码/线程契约、生命周期恢复、T_data与mask、三版本号、双轨切换、帧时风险和模块遥测已登记。验收仍由manager；实施仍等P0a真机确认。若上游后续改变σ/深度单位/光向语义或真实物理层序，先回designer/architect修订，developer不得自行修公式或放宽测试。

## 11. 变更记录

- 2026-10-24，v1：依据用户P-013 P0骨架独立卡交付开工级预设计。补齐完整帧原子接口、同步线程/生命周期协议、2048B上传、连续mask与坐标、可逆诊断灰阶、三版本登记和逐模块遥测。明确修正草案透明输出/灰阶饱和/成功返回歧义；保留manager验收与P0a真机双门槛。纯文书，零代码、零冻结锚点修改。
