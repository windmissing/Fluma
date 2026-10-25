# T20 新版基础GLES双场开发交付

日期：2026-10-25。按实施中抵达的T37/6089f98唯一合同，本报告替代developer-T20-particle-visual.md的“Canvas首选/等T37”出口说明；旧Canvas提交保留为完整同态退路。T34先提交2ef21f9后，才串行改controller/page/egl桥共享文件。

## 接口与行为

- 新physics/P011PublishedRenderState.ets；WaterSandField.createPublishedRenderStateSlot/copyPublishedRenderStateInto提供定容schema4场+完整稀疏池页，同一(runId,stateEpoch,physicsCommitId)。RESET/STEP/EXTERNAL_ADMISSION/INVERT/RESTORE/ROUTE_CHANGE发布；外源在子步之间成功准入也推进，zero/reject/rollback不发新token，restore/reset换epoch。非STEP不伪造ledgerCommitKey，真实endpoint stamp存在才复制key。单UI同步copy不调用step/collect，不向renderer暴露pool。
- 新P013DualFieldAdapter.packInto与双upload slot：扫描原槽升序压实state1/2；XY Float32、meta uint32 phase|(state<<8)+原ID，全容量对象复用，仅前count有效；验证schema4、token/count/capacity、正有限mass、Float32溢出、坐标/phase/state/ID/重复ID，异常整包拒绝。T_data复用原DISPLAY_4X3_V1，D只来自床层。
- 独立drawDualFieldFrame(frame,tdata,particleXY,particleMeta,visualParams)，P013_DUALFIELD_PACKET_V1/PARTICLE_XY_META_SOA_V1/DUALFIELD_BASIC_V1/BASIC_SRGB_DISPLAY_V1，不改旧drawFrame/P0枚举0–3。不借JSON转对象，不保留ArkTS backing pointer；类型/长度/offset/非detached/range/meta/值均先校验，再同步复制有效前缀至native staging。每capacity/epoch只分配两只VBO，粒子上传16×count字节，整数attribute+divisor1、六顶点quad实例化一次draw；count0/ablation不画粒子。
- 独立dualfield_pass.cpp/.h：T33实际sRGB attachment上透明清屏→基础相色沉积pass→真实粒子pass。显示sRGB候选颜色先EOTF再硬件编码一次；不吃E2 Lmat，不冒用P1 Reinhard或七版本。沿用原床相色及tracer相色、2px直径候选；混合带用基础中性色插值候选，不宣称完整P1或分相材质提升。shader FNV1A64、pipeline/color身份、count/capacity、uploadBytes、VBO分配/字节、clipped计数随receipt记录。
- 连续列/材料高度按同帧box与native尺度映射，visualInverted只做一次；controller拒非等比缩放，不重演物理镜像。FLYING读本帧T_data床遮挡；CONTACT_WAIT不被床mask/age删除、不吸附床面，保留实际位置与质量身份。MERGE/DEGRADE只反映池中现存记录。部分LAND余量继续入包，全额LAND后床保留、该sprite不再压实。
- 页面只有一次权威published copy。Canvas fallback的紧凑投影从该同一schema4池页派生；GPU成功时Canvas只画背景水/雾和前景雾/边框，关闭第二套沙床/粒子；C active始终关闭B feed/draw（含count0/ablation/失效），切入清B视觉缓存。GPU错误先隐藏整个TEXTURE及旧前景，用同一完整快照画Canvas真实粒子+床；route退出forceBaseline并释放，不误用G0 probe。readback仍仅G0 probe，新生产双场通路无readback。page hide不清物理池。
- route仍由M2既有显式routeId+params武装，默认off；页面不猜测/重写物理路由，不新增未裁定参数默认。资源/route切换、surface/context epoch不混用；失败不更新lastPresentedCommit。每秒WS_EVENT p013_dual_frame输出receipt，非GPU fence，不宣称同一次hardware present。

## 离线验证

- packet/publication 9/9：schema/token、洞槽/state2/ID、正质量/数值/meta反例、满池全量压实、两步间准入一次发布、zero/no-op、restore/invert、rollback不发布，复用数组不改物理。
- controller/composition 6/6（含双场成功、坏token/ablation不改库存）、Canvas fallback 9/9、P0 20/20、renderer82/82、gesture clock8/8。
- 九物理通道338/338，冻结断言不放宽。实际native bridge host新增有效offset视图、prefix自有拷贝、类型/长度/detached/range/非法meta/NaN/version/count0/context epoch反例；原P0/profile/lifecycle回归仍通过。host的DualFieldPass是GPU边界桩，真实dualfield_pass由arm64编译校验，shader运行/上传/像素/帧时不能由host推断。
- 工作区hvigor default@CompileArkTS default@BuildNativeWithNinja成功，零错误，旧警告保留。证据test/reports/t20-dual/；未用设备、未安装HAP。

## 独立已知物理故障（不伪报全场景通过）

新增满池动态探针test/probe_p011_full_pool_contact_failure.cjs：nmax16、同列重沙1.6unit、平床，step索引73返回code5，池余11个CONTACT_WAIT/重质量1.0226497308103744、H/L drift0。分别加载T34时2ef21f9的WaterSandField（--baseline）与当前版本复跑，失败步/池质量/漂移相同，原始双JSON保留；探针按失败返回exit1。此为原物理长跑缺陷，不改物理校验、不隐藏库存规避；已评论T20交manager另派M2返修/T30复验。满池静态打包、低负载真实LAND是独立绿色用例，不冒称该长跑通过。

本卡交付GLES基础显示实现与离线开发范围；T30/T31/T35负责真实GPU/端到端/平台透明sRGB/性能。已知物理问题及候选颜色/点径定标继续单列，不将开发交付等同产品指标通过。
