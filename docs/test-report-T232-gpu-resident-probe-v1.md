# T232 GPU 驻留平台真机探针 · 独立验证报告 v1

- 角色：tester（独立验证）· 日期：2026-10-28 · 状态：**结束待验收**
- 消费：`architecture-T231-gpu-resident-platform-v1.md` §5 探针矩阵 P-A..F + `algorithm-T230-gpu-resident-step-v1.md` §12 平台需求
- 被测：隔离探针工程 `.codex/local-test/t232-probe/`（基于 T171 v3 隔离工程 + 新增 native 探针），**不改生产开关/设备设置**
- 探针源：`test/p013_native/t232_resident_probe.cpp`；NAPI 包装 `t232_probe_napi.cpp`（runT232Probe 导出并入 pair binding 导出表，T232_PROBE 宏隔离，生产目标零影响）
- 部署包：`.codex/local-test/t232-probe/entry-default-t232-signed.hap`（v11）；执行入口 `aa start --ps t171runId t232probeA`
- 结果归档：`test/reports/t232/t232.json` + environment.txt + int_prim_log.txt + double_negative_log.txt

## 真机结果（VYG-AL00, Maleoon 920, GLES 3.2）—— 6/6 段落执行（P-E 显式 NOT_RUN）

| 探针 | 内容 | 结果 |
|---|---|---|
| identity | GL/GLSL/vendor/renderer | GLES 3.2 / GLSL 3.20 / HUAWEI Maleoon 920（3.2） |
| **P-B limits** | 扩展 limits + offset alignment | globalSSBO=32, compute=16, combined=32, vertex=0, fragment=16, **offsetAlign=256**, wgCount=65535³, wgSize=1024³, invocations=1024, ssboMaxBytes=1GiB, shared=32KiB |
| extensions | gpu_shader5/fp64/int64/atomicFloat/timer | gpu_shader5=1, **fp64Hint=0, int64=0**（GRS 整数核路径不可依赖 GL int64）, atomicFloat=0, disjointTimerQuery=1 |
| doubleCompileNegative | 负控：非法 double shader 被拒 | rejected=1 ✓ |
| **P-A 整数原语** | uaddCarry/usubBorrow/umulExtended/移位/RNE 除 vs CPU oracle，N=4096 含 4 边界 | **OK mismatch=0**（uvec4(carry,borrow,hi,lo)+mix 逐位一致） |
| **P-C 跨 dispatch** | writer(+7, 2 workgroup)→barrier→reader 校验 | **OK bad=0/128**（SSBO barrier 语义正确） |
| **P-D completion** | fence 状态/poll/等待 | **OK**：fenceStatus=37146(GL_ALREADY_SIGNALED), polls=1, fenceWaitMs=0.003, tailBad=0/256 |
| P-E 共享 context | compute→scene 同 context 交接 | NOT_RUN_PBUFFER_SECTION（需 XComponent 真实 window context，归后续设备段） |

## 关键平台结论（供 T230/T231 工程决策）

1. **整数核可行**：uaddCarry/usubBorrow/umulExtended/RNE 除语义在 Maleoon 920 与 CPU oracle 完全一致 → T230 Q20 固定点路径硬件原语齐备。
2. **无 64 位整数扩展**（int64=0）→ GRS 设计按 32 位原语组合实现 64 位乘加（umulExtended 已验证），不得使用 GL int64 类型。
3. **SSBO offset 对齐 256B** → 驻留 buffer 布局按 256B 对齐切分区段；vertex SSBO=0 → 顶点着色器不可读 SSBO。
4. **跨 dispatch barrier 正确** → GPU 驻留状态机（状态保留在 SSBO、逐帧 dispatch 推进）的内存一致性前提成立。
5. **1 GiB SSBO 上限** ≥ 设计需求的 15/37MB 驻留资源（复核通过）；wgSize 1024 足够。
6. 计时器扩展可用（disjointTimerQuery=1），供后续吞吐探针。

## 执行过程要点（诚实记录）

- 首跑 rc=3（P-A COMPILE_FAILED）：①驱动严格类型检查 `a.length()`/`i % b.length()` 需显式 `uint()` 化（GLES 3.2 Maleoon 编译器对 uint/int 混算拒绝，桌面驱动常宽松）——已修；②P-C reader 判据未计入 writer +7 变换（测试构造错误，非平台缺陷）——已修。
- `g_out` 未赋值导致 dumpText 落相对路径——已修（`g_out=outDir`）。
- NAPI 导出并入 pair binding（独立 napi_module 注册不生效）；worker hook 前缀匹配 blockId。
- 构建缓存坑：ArkTS worker / native .so 增量不重编导致多轮"假成功"，验证手段=zip 内 abc/so 字符串断言（`t232-hook-hit`、`runT232Probe`）+ 干净 uninstall/install。
- `ets/workers/T171Worker.abc` zip_file GetEntry 错误为良性（运行时回退 modules.abc，主工程同现象）。

## 边界

- P-E（compute→scene 共享 context）需 XComponent 真实 window context，本段 pbuffer 不可达，未宣称完成。
- 吞吐/帧率测量不在本卡（T230 §12 P-F 仅扩展探测；专项性能卡另开）。
- 探针为 test-only，生产 CMake 目标不含 t232 文件（T232_PROBE 宏隔离 + pair binding 导出表不加生产符号）。

## 失败上报

无阻断。P-E 交接后续设备段（XComponent 专用卡）。
