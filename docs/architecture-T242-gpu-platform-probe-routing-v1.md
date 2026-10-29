# T242 GPU 平台探针接续工程与证据边界

> **T255 接续修订（2026-10-29）**：T246 已编译独立 SURFACE 窗口示意探针、六档实际 buffer 与三目标资源分配代码及 1024B 探针 tail，均未真机执行。其 `T246` tail 只存 owner/context/surface/lease/token 与 done，不能替 T241 的产品 completion256、审计、物理 commit。独立窗口/资源/真实生命周期探针可以与 T236 离线 M0 并行；只有产品 completion→native accept→publication lease 的集成必须等实际 M0 接口。真实迟到、后台/重建和 context loss 要靠发生过的 callback/GL/fence 证据，不可用 hook 改内存 token 代替。详见 `architecture-T255-window-probe-remainder-v1.md`。

2026-10-29，architect；原卡 `6ae28b60e4b0c3103c1983fa`。首次归因消费 T232 正式报告及 `test/reports/t232/t232.json`、T239 WM11，并修订 T231/T233/T238。**工程合同和设备取证范围已明确；真实窗口驻留可行性仍待证。** 不使用真机、不重跑测试、不改 endpoint 或业务代码。

## 已证与未证

- T232 是私有 pbuffer：4096 整数原语零错，跨 dispatch barrier 128 项零错，256 word/1024B 尾部微探针零错；其 completion 并非 M0/M1 产品证书字段、整 root 审计或五 root/tick。`GL_ALREADY_SIGNALED` 一次 poll、0.003437ms fence wait 只属于微探针。
- 查询值 globalBindings32、computeBlocks16、combinedBlocks32、vertexBlocks0、fragmentBlocks16、offsetAlignment256、单 block 1GiB。256B 是 range **起点**对齐，不是 U32 内部每个 word 对齐。1GiB 限值不等于任意多对象成功分配或可共存。`P-E=NOT_RUN_PBUFFER_SECTION`，没有真实 XComponent window 同 context compute→draw/swap、双向共享 context、1024B 产品尾包或完整帧性能证据。
- T233/T238 payload 算术分别 M0 K192/512 为 15,060,496/36,997,904B，M1 Kw64 为 23,343,248/55,111,056B、Kw128 为 25,472,400/57,240,208B。以上还要加对象 alignment、program、staging、双全帧 target、LastGood/lease、CPU oracle/capture 与 driver/RSS，不能把设计表写成设备峰值。
- T239 WM11 的四档常数与 `h=ceil(log2(nextPow2(K)))`、`L=h(h+1)/2` 一致；原 `hK` 记法含糊，T238 已明确 h/Lw/四档 quantum/tick。没有可证设计矛盾；若实际拆 pass，须另版 dispatch/资源账。

## 接力任务书（manager 据此建/更新卡）

**平台 developer，离线工程先行且不使用真机**：在隔离新 XComponent+shader route 增最小真实窗口探针入口，不改生产开关；唯一 GL owner 同 context 提交整数 compute→正确 consumer barrier→fragment 整数 SSBO fetch→完整背景/前景/物理场景 draw/swap，并输出同 token、owner/context/surface epoch、rawGL/EGL、shader/source SHA、帧呈现状态。vertex SSBO=0 时不能默认走 vertex fetch；若 fragment 失败，另版 GPU image/texture 或 VBO pack，计实际 pass/bytes/barrier，不做全池 CPU 往返。加入 256B range offset 成功/错位拒绝、1024B 产品字段形状尾包、候选/发布 lease 与 resize/context loss 后旧代拒绝的探针钩子。工程代码编译与非真机自测由 developer 完成；实际设备执行另交 tester。

**tester 真机独立补证，设备串行、GPU 优先**：在与正式渲染一致的 window GL context 重查各 stage limits；分档以 **实际 `glBufferData/Storage` 成功及可读写**为准分配 M0、M1 Kw64/128 的三 bank、双 event、双 publication、completion、program/staging 和完整 `8*w*h` 及 LastGood 共存资源，记每对象 live/peak/offset/size、raw GL error/清理。先最小 compute→fragment/draw/swap同 token，再完整 1024B 尾包迟到、取消/lease、resize/background/context lost 旧代拒绝；若要共享 context，实际 share group 加双向 fence 才能声称可用。记录完整 frame 与 GPU timer disjoint，不能仅报微核性能。故障只给 FAULT 或标明 STALE 的上一完整有效新管线帧；背景/前景也禁止 Canvas。P-E 未执行与内存未分配时保持 NOT_MEASURED/CAPABILITY_REQUIRED，不能转产品 PASS。

T236 独立离线 M0 开发继续，不把上述设备窗口当其前置；T240 仍按真实 M0 接口前置。若窗口探针失败，原始错误交 architect 再作工程/设计分流，不预判硬件不可行。
