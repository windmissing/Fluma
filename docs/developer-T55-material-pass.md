# T55 G3 完整材质 pass 交付

2026-10-26，developer。不使用真机；没有产品页面集成或 Canvas 上屏/回退新增。依据 P1 rev2 与签收配方，消费 T68 新管线责任边界。T67 上传器补充另会话适配，其 hunk 不混入本提交。

## 交付

- `material_pass.{h,cpp}` 与 `glsl/p1_material.vert/.frag`：layout2 128×3，连续列采样 S/pb/fH/warp/spark；H/L/M 三带；低端 palette、M按 fH 在编码域混色，gain→clamp248→round；B/C1/C3 使用签收 Q 增量；只在总和做一次下界 clamp，共用一次 Lmat 与 ToneMap。
- OFF绘制完整同材质、Lmat=1，不受P0 diagnosticEnabled控制；B/C/spark只消融相应项，C同时关C1/C3，不改资产/种子。C3保留206/159两块样式及真实阶跃；正mod、pre-invert坐标、一次镜像；没有纹理模糊或第二光场。
- 复用同一个 T24 GLSL helper。CMake嵌入精确源码并 SHA256，统一换行；宿主实际编译与native构建 hash一致：`204c7fe5f78005eee50954ff2f3f19b368dd8d88968fade9fc344eba0d68b2e6`。
- `MaterialPass::Draw` 是T68候选沙层：不清除整个target，二值coverage以线性显示域ONE/ONE_MINUS_SRC_ALPHA over保留已有背景。静态读回自测验证空沙层不覆盖原背景。T56负责双全帧FBO、其它场景层、LastGood和成功swap后scene lease可见代提交。
- 新 `drawMaterialFrame(frame,tdataV2,light,material)`、强类型输入/参数/回执和独立controller方法；精确元素类型/长度/offset/detach、七版本、fH/保留通道、有限值、同代配置/geometry/context/表面、单调frameId检查。共享P0 session消费者，但不改P0 drawFrame。READY候选仅在本串行隔离调用内预检后激活；完整scene的pin/after-swap提交仍归T56，不能把本隔离API作为新产品scene交易。
- 回执明确 `P013_MATERIAL_ISOLATED_V1 / sceneTransactionImplemented=false`，失败是fault且本帧swap=false；stale=false，因为本模块没有自有LastGood纹理，绝不凭旧swap伪造可恢复stale。requested与actual代分列；保留lastSwap仅作历史。无Canvas退路，无帧时/物理修改。

## 可复跑证据

1. `node test/run_p055_material_units.cjs`：6/6；类型、OFF、代/token拒绝、shader/helper身份与无clear责任。原T24 11例中数学10/10；准备阶段“未嵌入shader”断言在G3授权后预期失效，原测试/期望不改，新报告显式 SUPERSEDED_BY_AUTHORIZED_G3_IMPLEMENTATION。
2. 原 `p013_cpu_reference_execution.py` 在独立输出路径复跑58/58，未覆盖tester原报告；核/pb32/解析颜色保持。
3. `test/run_p055_material_gpu.ps1 -Target srgb-fbo`：真实Windows ANGLE GLES3/Intel UHD630，生产MaterialPass+MaterialGlGpu、真实T48生成器导出六图（仅离线CPU导出，无Canvas上屏），35组OFF/ON×B/C/spark及坡面/脉冲/空/薄/镜像、0/255极端Q夹具。所有binary alpha/沙外零结构检查PASS，OFF换光向逐字节相同、OFF非透明。独立SRGB8_ALPHA8 FBO对CPU最大code误差约1.9563/255；各fixture RMS/max/worst坐标、数组摘要均已输出。未冻结数值通过线，只标MEASURED_NOT_CALIBRATED。
4. `-Target default-pbuffer`：同宿主EGL_SRGB默认pbuffer报告attachment=GL_SRGB，但输出读回与CPU链最大误差约73.7203/255，明确为待归因的宿主目标色彩差异，**不能声明surface/屏幕色彩已通过**。没有加gamma兜底或调整配方。FBO成功不替代设备surface A/B/C证据；两目标分别保存。
5. `phase-boundaries.json` 保留连续warp→离散C texel相邻像素与C3中点样本，不删阶跃、不模糊；不设新prominence/相位台阶合格线。
6. P0 20/20、composition6/6，原路径仍绿。hvigor CompileArkTS/BuildNativeWithNinja/PackageHap BUILD SUCCESSFUL，零错误，原警告保留。
7. 新宿主bridge用例源码含READY原子候选、代/revision/context/版本/长度/域反例。早期隔离版本曾执行PASS；最新程序编译链接成功后OS报Access is denied，产物消失，原因未经确认，**本轮最新bridge运行记ENVIRONMENT_BLOCKED而非PASS**。未绕过系统保护。实际GLES执行器可运行且结果如上，不冒充Ark/设备bridge。

证据目录 `test/reports/t55/`。宿主shader/readback为真实GL执行，平台/NAPI桥历史桩与实际GPU分开；无真机安装、性能、最终屏幕或视觉定标签字。

## 交接与剩余边界

T55完成材质层开发/编译/本机可执行自测；默认pbuffer颜色偏差交designer归因/必要tester补证，最新host bridge环境阻断需独立重跑。T56消费MaterialPass构建全场纯shader，按T67绑定与T68 AcquireSceneLease/CommitSceneLeaseAfterSwap接合，不使用本隔离API替代scene LastGood。完整背景/水/雾/高光/边框、产品模式、整场失败恢复、设备同帧/颜色/性能仍未实现或未测，不在此宣称G3产品/视觉通过。
